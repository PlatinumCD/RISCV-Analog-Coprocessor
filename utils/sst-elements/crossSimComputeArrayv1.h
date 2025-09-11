// CrossSimComputeArray.h (thread-safe w/ GIL guards)
#ifndef _CROSSSIMCOMPUTEARRAY_H
#define _CROSSSIMCOMPUTEARRAY_H

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <sst/elements/golem/array/computeArray.h>
#include <Python.h>
#include "numpy/arrayobject.h"

#include <atomic>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>
#include <algorithm>

namespace SST {
namespace Golem {

struct GILGuard {
    PyGILState_STATE state;
    GILGuard()  { state = PyGILState_Ensure(); }
    ~GILGuard() { PyGILState_Release(state);   }
};

template<typename T>
class CrossSimComputeArray : public ComputeArray {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(
        CrossSimComputeArray<T>,
        SST::Golem::ComputeArray,
        TimeConverter*,
        Event::HandlerBase*
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"CrossSimJSONParameters", "JSON configuration for CrossSim", "default"}
    )

    CrossSimComputeArray(ComponentId_t id, Params& params,
                         TimeConverter* tc,
                         Event::HandlerBase* handler)
        : ComputeArray(id, params, tc, handler)
    {
        initializePython();
        CrossSimJSON = params.find<std::string>("CrossSimJSONParameters");

        selfLink = configureSelfLink("Self", tc,
            new Event::Handler2<CrossSimComputeArray,&CrossSimComputeArray::handleSelfEvent>(this));
        selfLink->setDefaultTimeBase(latencyTC);

        pyMatrix = new PyObject*[numArrays];
        npMatrix = new PyArrayObject*[numArrays];
        pyArrayIn = new PyObject*[numArrays];
        npArrayIn = new PyArrayObject*[numArrays];
        pyArrayOut = new PyObject*[numArrays];
        npArrayOut = new PyArrayObject*[numArrays];
        cores = new PyObject*[numArrays];
        setMatrixFunction = new PyObject*[numArrays];
        computeMVM = new PyObject*[numArrays];

        inputVectors.resize(numArrays);
        outputVectors.resize(numArrays);
        for (uint32_t i = 0; i < numArrays; i++) {
            inputVectors[i].resize(inputArraySize, T());
            outputVectors[i].resize(outputArraySize, T());
        }
    }

    virtual ~CrossSimComputeArray() {
        // All Python refcount ops must hold the GIL.
        {
            GILGuard g;
            for (uint32_t i = 0; i < numArrays; i++) {
                Py_XDECREF(pyMatrix[i]);
                Py_XDECREF(pyArrayIn[i]);
                Py_XDECREF(pyArrayOut[i]);
                Py_XDECREF(cores[i]);
                Py_XDECREF(setMatrixFunction[i]);
                Py_XDECREF(computeMVM[i]);
            }
            Py_XDECREF(crossSim);
            Py_XDECREF(paramsConstructor);
            Py_XDECREF(AnalogCoreConstructor);
            Py_XDECREF(crossSim_params);
        }

        delete[] pyMatrix;
        delete[] npMatrix;
        delete[] pyArrayIn;
        delete[] npArrayIn;
        delete[] pyArrayOut;
        delete[] npArrayOut;
        delete[] cores;
        delete[] setMatrixFunction;
        delete[] computeMVM;

        finalizePython();
    }

    virtual void init(unsigned int phase) override {
        if (phase != 0) return;

        uint64_t inputSize = inputArraySize;
        uint64_t outputSize = outputArraySize;

        int matrixNumDims = 2;
        npy_intp matrixDims[2] = { static_cast<npy_intp>(outputSize),
                                   static_cast<npy_intp>(inputSize) };

        int arrayInNumDims = 1;
        npy_intp arrayInDim[1] = { static_cast<npy_intp>(inputSize) };

        int arrayOutNumDims = 1;
        npy_intp arrayOutDim[1] = { static_cast<npy_intp>(outputSize) };

        int numpyType = getNumpyType();

        {
            GILGuard g;

            // Create NumPy arrays
            for (uint32_t i = 0; i < numArrays; i++) {
                pyMatrix[i]  = PyArray_SimpleNew(matrixNumDims, matrixDims, numpyType);
                npMatrix[i]  = reinterpret_cast<PyArrayObject*>(pyMatrix[i]);

                pyArrayIn[i] = PyArray_SimpleNew(arrayInNumDims, arrayInDim, numpyType);
                npArrayIn[i] = reinterpret_cast<PyArrayObject*>(pyArrayIn[i]);

                // Allocate an output array holder (will be replaced by matvec result)
                pyArrayOut[i] = PyArray_SimpleNew(arrayOutNumDims, arrayOutDim, numpyType);
                npArrayOut[i] = reinterpret_cast<PyArrayObject*>(pyArrayOut[i]);
            }

            // Import simulator module & constructors
            crossSim = PyImport_ImportModule("simulator");
            if (!crossSim) { out.fatal(CALL_INFO, -1, "Import CrossSim failed\n"); PyErr_Print(); }

            paramsConstructor = PyObject_GetAttrString(crossSim, "CrossSimParameters");
            if (!paramsConstructor) { out.fatal(CALL_INFO, -1, "Get CrossSimParameters failed\n"); PyErr_Print(); }

            AnalogCoreConstructor = PyObject_GetAttrString(crossSim, "AnalogCore");
            if (!AnalogCoreConstructor) { out.fatal(CALL_INFO, -1, "Get AnalogCore failed\n"); PyErr_Print(); }

            // Build parameters
            if (CrossSimJSON.empty()) {
                crossSim_params = PyObject_CallFunction(paramsConstructor, NULL);
            } else {
                PyObject* fromJson = PyObject_GetAttrString(paramsConstructor, "from_json");
                PyObject* jsonArgs = Py_BuildValue("(s)", CrossSimJSON.c_str());
                crossSim_params = PyObject_CallObject(fromJson, jsonArgs);
                Py_XDECREF(fromJson);
                Py_XDECREF(jsonArgs);
            }
            if (!crossSim_params) { out.fatal(CALL_INFO, -1, "CrossSimParameters ctor failed\n"); PyErr_Print(); }

            if constexpr (std::is_same_v<T, int64_t>) {
                PyObject* core = PyObject_GetAttrString(crossSim_params, "core");
                if (!core) { out.fatal(CALL_INFO, -1, "params.core missing\n"); PyErr_Print(); }
                else {
                    PyObject* dtypeValue = PyUnicode_FromString("INT64");
                    if (PyObject_SetAttrString(core, "output_dtype", dtypeValue) != 0) {
                        out.fatal(CALL_INFO, -1, "set core.output_dtype failed\n"); PyErr_Print();
                    }
                    Py_DECREF(dtypeValue);
                    Py_DECREF(core);
                }
            }

            // Create cores & bind methods
            for (uint32_t i = 0; i < numArrays; i++) {
                cores[i] = PyObject_CallFunctionObjArgs(AnalogCoreConstructor,
                                                        pyMatrix[i],
                                                        crossSim_params,
                                                        NULL);
                if (!cores[i]) { out.fatal(CALL_INFO, -1, "AnalogCore() failed\n"); PyErr_Print(); }

                setMatrixFunction[i] = PyObject_GetAttrString(cores[i], "set_matrix");
                if (!setMatrixFunction[i]) { out.fatal(CALL_INFO, -1, "core.set_matrix missing\n"); PyErr_Print(); }

                computeMVM[i] = PyObject_GetAttrString(cores[i], "matvec");
                if (!computeMVM[i]) { out.fatal(CALL_INFO, -1, "core.matvec missing\n"); PyErr_Print(); }
            }
        }
    }

    virtual void beginComputation(uint32_t arrayID) override {
        SimTime_t latency = getArrayLatency(arrayID);
        ArrayEvent* ev = new ArrayEvent(arrayID);
        selfLink->send(latency, ev);
    }

    virtual void handleSelfEvent(Event* ev) override {
        ArrayEvent* aev = static_cast<ArrayEvent*>(ev);
        uint32_t arrayID = aev->getArrayID();
        compute(arrayID);
        (*tileHandler)(ev);
    }

    virtual void setMatrixItem(int32_t arrayID, int32_t index, double value) override {
        GILGuard g;
        T* data = reinterpret_cast<T*>(PyArray_DATA(npMatrix[arrayID]));
        data[index] = static_cast<T>(value);

        if (index == static_cast<int32_t>(inputArraySize * outputArraySize - 1)) {
            PyObject* status = PyObject_CallFunctionObjArgs(setMatrixFunction[arrayID],
                                                            npMatrix[arrayID], NULL);
            if (!status) { out.fatal(CALL_INFO, -1, "core.set_matrix failed\n"); PyErr_Print(); }
            Py_XDECREF(status);
        }
    }

    virtual void setVectorItem(int32_t arrayID, int32_t index, double value) override {
        GILGuard g;
        T* data = reinterpret_cast<T*>(PyArray_DATA(npArrayIn[arrayID]));
        data[index] = static_cast<T>(value);
    }

    virtual void compute(uint32_t arrayID) override {
        GILGuard g;

        PyObject* res = PyObject_CallFunctionObjArgs(computeMVM[arrayID],
                                                     npArrayIn[arrayID], NULL);
        if (!res) { out.fatal(CALL_INFO, -1, "matvec() failed\n"); PyErr_Print(); }
        Py_XDECREF(pyArrayOut[arrayID]);              // drop previous holder (if any)
        pyArrayOut[arrayID] = res;
        npArrayOut[arrayID] = reinterpret_cast<PyArrayObject*>(pyArrayOut[arrayID]);

        int len = static_cast<int>(PyArray_SIZE(npArrayOut[arrayID]));
        outputVectors[arrayID].resize(len);
        T* outputData = reinterpret_cast<T*>(PyArray_DATA(npArrayOut[arrayID]));
        std::copy(outputData, outputData + len, outputVectors[arrayID].begin());

        // Debug prints (safe while GIL held)
        out.verbose(CALL_INFO, 2, 0, "CrossSim MVM on array %u:\n", arrayID);
        T* inputData  = reinterpret_cast<T*>(PyArray_DATA(npArrayIn[arrayID]));
        T* matrixData = reinterpret_cast<T*>(PyArray_DATA(npMatrix[arrayID]));
        for (uint32_t col = 0; col < inputArraySize; col++) printValue(inputData[col]);
        out.verbose(CALL_INFO, 2, 0, "\n\n");
        for (uint32_t row = 0; row < outputArraySize; row++) {
            for (uint32_t col = 0; col < inputArraySize; col++)
                printValue(matrixData[row * inputArraySize + col]);
            out.verbose(CALL_INFO, 2, 0, "  ");
            printValue(outputData[row]);
            out.verbose(CALL_INFO, 2, 0, "\n");
        }
        out.verbose(CALL_INFO, 2, 0, "\n\n");
    }

    virtual SimTime_t getArrayLatency(uint32_t) override { return 1; }

    virtual void moveOutputToInput(uint32_t srcArrayID, uint32_t destArrayID) override {
        GILGuard g;
        T* src = reinterpret_cast<T*>(PyArray_DATA(npArrayOut[srcArrayID]));
        T* dst = reinterpret_cast<T*>(PyArray_DATA(npArrayIn[destArrayID]));
        std::copy(src, src + outputArraySize, dst);
    }

    virtual void* getInputVector(uint32_t arrayID) override {
        GILGuard g;
        T* data = reinterpret_cast<T*>(PyArray_DATA(npArrayIn[arrayID]));
        int len = static_cast<int>(PyArray_SIZE(npArrayIn[arrayID]));
        inputVectors[arrayID].resize(len);
        std::copy(data, data + len, inputVectors[arrayID].begin());
        return static_cast<void*>(&inputVectors[arrayID]);
    }

    virtual void* getOutputVector(uint32_t arrayID) override {
        return static_cast<void*>(&outputVectors[arrayID]);
    }

protected:
    std::string CrossSimJSON;

    PyObject* crossSim              = nullptr;
    PyObject* paramsConstructor     = nullptr;
    PyObject* AnalogCoreConstructor = nullptr;
    PyObject* crossSim_params       = nullptr;

    PyObject**      pyMatrix          = nullptr;
    PyArrayObject** npMatrix          = nullptr;
    PyObject**      pyArrayIn         = nullptr;
    PyArrayObject** npArrayIn         = nullptr;
    PyObject**      pyArrayOut        = nullptr;
    PyArrayObject** npArrayOut        = nullptr;
    PyObject**      cores             = nullptr;
    PyObject**      setMatrixFunction = nullptr;
    PyObject**      computeMVM        = nullptr;

    std::vector<std::vector<T>> inputVectors;
    std::vector<std::vector<T>> outputVectors;

    int getNumpyType() {
        if constexpr (std::is_same<T, int64_t>::value)      return NPY_INT64;
        else if constexpr (std::is_same<T, float>::value)   return NPY_FLOAT32;
        else { static_assert(!sizeof(T*), "Unsupported data type"); }
    }

    void printValue(const T& v) {
        if constexpr (std::is_same<T, int64_t>::value)      out.verbose(CALL_INFO, 2, 0, "%ld ", v);
        else if constexpr (std::is_same<T, float>::value)   out.verbose(CALL_INFO, 2, 0, "%f ", v);
    }

private:
    // --- Process-wide Python lifetime management ---
    static std::atomic<int>& instanceCount() {
        static std::atomic<int> c{0};
        return c;
    }
    static std::once_flag& initFlag() {
        static std::once_flag f;
        return f;
    }
    static PyThreadState*& mainState() {
        static PyThreadState* s = nullptr;
        return s;
    }

    static void doPythonInitialization() {
        Py_Initialize();
        PyEval_InitThreads();   // ensure GIL machinery is ready (no-op on 3.11+, still safe)
        import_array1();         // initialize NumPy C-API (requires GIL)
        // Release the GIL so worker threads can acquire it on demand
        mainState() = PyEval_SaveThread();
    }

    void initializePython() {
        std::call_once(initFlag(), doPythonInitialization);
        instanceCount().fetch_add(1, std::memory_order_acq_rel);
    }

    void finalizePython() {
        if (instanceCount().fetch_sub(1, std::memory_order_acq_rel) == 1) {
            // Last user: reacquire GIL held by mainState and finalize
            if (mainState()) {
                PyEval_RestoreThread(mainState());
                mainState() = nullptr;
            }
            Py_Finalize();
        }
    }
};

} // namespace Golem
} // namespace SST

#endif /* _CROSSSIMCOMPUTEARRAY_H */


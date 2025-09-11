// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory
// of the distribution.

#ifndef _CROSSSIMCOMPUTEARRAY_H
#define _CROSSSIMCOMPUTEARRAY_H

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <sst/elements/golem/array/computeArray.h>
#include <Python.h>
#include "numpy/arrayobject.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <iostream>

namespace SST {
namespace Golem {

// ---------- thread-safe logging ----------
static std::mutex& logMutex() { static std::mutex m; return m; }
static uint64_t tid_u64() { return std::hash<std::thread::id>{}(std::this_thread::get_id()); }

static void gil_log(const char* msg) {
    std::lock_guard<std::mutex> lk(logMutex());
    std::cout << "[GIL] thread " << tid_u64() << " " << msg << std::endl;
}
static void wf_log(const std::string& msg) {
    std::lock_guard<std::mutex> lk(logMutex());
    std::cout << "[WF ] thread " << tid_u64() << " " << msg << std::endl;
}

// ---------- RAII helpers for Python GIL ----------
struct GILGuard {
    PyGILState_STATE s;
    GILGuard()  { s = PyGILState_Ensure(); gil_log("acquired GIL"); }
    ~GILGuard() { gil_log("released GIL"); PyGILState_Release(s);   }
};

// Use ONLY while the GIL is held; temporarily releases it for heavy work.
struct NoGIL {
    PyThreadState* save;
    NoGIL()  : save(PyEval_SaveThread()) { gil_log("temporarily released GIL"); }
    ~NoGIL() { PyEval_RestoreThread(save); gil_log("reacquired GIL"); }
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
        wf_log("ctor: start");
        initializePython();
        CrossSimJSON = params.find<std::string>("CrossSimJSONParameters");

        selfLink = configureSelfLink("Self", tc,
            new Event::Handler2<CrossSimComputeArray,&CrossSimComputeArray::handleSelfEvent>(this));
        selfLink->setDefaultTimeBase(latencyTC);

        wf_log("ctor: allocate Python object arrays");
        pyMatrix = new PyObject*[numArrays];
        npMatrix = new PyArrayObject*[numArrays];
        pyArrayIn = new PyObject*[numArrays];
        npArrayIn = new PyArrayObject*[numArrays];
        pyArrayOut = new PyObject*[numArrays];
        npArrayOut = new PyArrayObject*[numArrays];
        cores = new PyObject*[numArrays];
        setMatrixFunction = new PyObject*[numArrays];
        computeMVM = new PyObject*[numArrays];

        wf_log("ctor: allocate host-side buffers");
        inputVectors.resize(numArrays);
        outputVectors.resize(numArrays);
        hostMatrix.resize(numArrays);
        hostInput.resize(numArrays);

        for (uint32_t i = 0; i < numArrays; i++) {
            inputVectors[i].resize(inputArraySize, T());
            outputVectors[i].resize(outputArraySize, T());
            hostMatrix[i].resize(inputArraySize * outputArraySize, T());
            hostInput[i].resize(inputArraySize, T());
        }
        wf_log("ctor: done");
    }

    virtual ~CrossSimComputeArray() {
        wf_log("dtor: start");
        // Drop Python references while holding the GIL.
        if (Py_IsInitialized()) {
            GILGuard g;
            wf_log("dtor: Py_DECREF Python objects");
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

        wf_log("dtor: free C++ arrays");
        delete[] pyMatrix;
        delete[] npMatrix;
        delete[] pyArrayIn;
        delete[] npArrayIn;
        delete[] pyArrayOut;
        delete[] npArrayOut;
        delete[] cores;
        delete[] setMatrixFunction;
        delete[] computeMVM;

        wf_log("dtor: finalizePython()");
        finalizePython();
        wf_log("dtor: done");
    }

    virtual void init(unsigned int phase) override {
        if (phase != 0) return;
        wf_log("init[phase0]: start");

        const uint64_t inputSize  = inputArraySize;
        const uint64_t outputSize = outputArraySize;

        const int matrixNumDims = 2;
        npy_intp matrixDims[2] = {
            static_cast<npy_intp>(outputSize),
            static_cast<npy_intp>(inputSize)
        };

        const int arrayInNumDims = 1;
        npy_intp arrayInDim[1] = { static_cast<npy_intp>(inputSize) };

        const int arrayOutNumDims = 1;
        npy_intp arrayOutDim[1] = { static_cast<npy_intp>(outputSize) };

        const int numpyType = getNumpyType();

        {
            GILGuard g;

            wf_log("init[phase0]: allocate NumPy arrays");
            for (uint32_t i = 0; i < numArrays; i++) {
                pyMatrix[i]  = PyArray_SimpleNew(matrixNumDims, matrixDims, numpyType);
                npMatrix[i]  = reinterpret_cast<PyArrayObject*>(pyMatrix[i]);

                pyArrayIn[i] = PyArray_SimpleNew(arrayInNumDims, arrayInDim, numpyType);
                npArrayIn[i] = reinterpret_cast<PyArrayObject*>(pyArrayIn[i]);

                pyArrayOut[i] = PyArray_SimpleNew(arrayOutNumDims, arrayOutDim, numpyType);
                npArrayOut[i] = reinterpret_cast<PyArrayObject*>(pyArrayOut[i]);
            }

            wf_log("init[phase0]: import simulator module");
            crossSim = PyImport_ImportModule("simulator");
            if (!crossSim) { out.fatal(CALL_INFO, -1, "Import CrossSim failed\n"); PyErr_Print(); }

            wf_log("init[phase0]: get constructors");
            paramsConstructor = PyObject_GetAttrString(crossSim, "CrossSimParameters");
            if (!paramsConstructor) { out.fatal(CALL_INFO, -1, "Get CrossSimParameters failed\n"); PyErr_Print(); }

            AnalogCoreConstructor = PyObject_GetAttrString(crossSim, "AnalogCore");
            if (!AnalogCoreConstructor) { out.fatal(CALL_INFO, -1, "Get AnalogCore failed\n"); PyErr_Print(); }

            wf_log("init[phase0]: build parameters");
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
                wf_log("init[phase0]: set core.output_dtype=INT64");
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

            wf_log("init[phase0]: create cores & bind methods");
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

        wf_log("init[phase0]: done");
    }

    virtual void beginComputation(uint32_t arrayID) override {
        wf_log("beginComputation: schedule self event for array " + std::to_string(arrayID));
        SimTime_t latency = getArrayLatency(arrayID);
        ArrayEvent* ev = new ArrayEvent(arrayID);
        selfLink->send(latency, ev);
    }

    virtual void handleSelfEvent(Event* ev) override {
        ArrayEvent* aev = static_cast<ArrayEvent*>(ev);
        uint32_t arrayID = aev->getArrayID();
        wf_log("handleSelfEvent: compute array " + std::to_string(arrayID));
        compute(arrayID);
        wf_log("handleSelfEvent: invoking tileHandler");
        (*tileHandler)(ev);
    }

    // Host-side write; on last element, bulk copy to NumPy + call set_matrix under GIL.
    virtual void setMatrixItem(int32_t arrayID, int32_t index, double value) override {
        hostMatrix[arrayID][index] = static_cast<T>(value);

        const int32_t last = static_cast<int32_t>(inputArraySize * outputArraySize - 1);
        if (index == last) {
            wf_log("setMatrixItem: last element → commit matrix " + std::to_string(arrayID));
            GILGuard g;

            T* mptr = reinterpret_cast<T*>(PyArray_DATA(npMatrix[arrayID]));
            {
                NoGIL nogil; // release GIL during memcpy
                std::memcpy(mptr,
                            hostMatrix[arrayID].data(),
                            hostMatrix[arrayID].size() * sizeof(T));
            }

            PyObject* status = PyObject_CallFunctionObjArgs(setMatrixFunction[arrayID],
                                                            npMatrix[arrayID], NULL);
            if (!status) { out.fatal(CALL_INFO, -1, "core.set_matrix failed\n"); PyErr_Print(); }
            Py_XDECREF(status);
            wf_log("setMatrixItem: set_matrix complete for array " + std::to_string(arrayID));
        }
    }

    // Host-side write only; copied to NumPy just-in-time in compute().
    virtual void setVectorItem(int32_t arrayID, int32_t index, double value) override {
        hostInput[arrayID][index] = static_cast<T>(value);
        // wf_log("setVectorItem: array " + std::to_string(arrayID) + " idx " + std::to_string(index));
    }

    virtual void compute(uint32_t arrayID) override {
        wf_log("compute: start array " + std::to_string(arrayID));

        // Pointers to NumPy buffers; we copy without GIL where safe.
        T* inPtr  = nullptr;
        T* outPtr = nullptr;
        int outLen = 0;

        {
            GILGuard g;

            wf_log("compute: copy host → NumPy input");
            inPtr = reinterpret_cast<T*>(PyArray_DATA(npArrayIn[arrayID]));
            {
                NoGIL nogil;
                std::memcpy(inPtr,
                            hostInput[arrayID].data(),
                            hostInput[arrayID].size() * sizeof(T));
            }

            wf_log("compute: call core.matvec()");
            PyObject* res = PyObject_CallFunctionObjArgs(computeMVM[arrayID],
                                                         npArrayIn[arrayID], NULL);
            if (!res) { out.fatal(CALL_INFO, -1, "matvec() failed\n"); PyErr_Print(); }

            Py_XDECREF(pyArrayOut[arrayID]);            // drop previous result (if any)
            pyArrayOut[arrayID] = res;
            npArrayOut[arrayID] = reinterpret_cast<PyArrayObject*>(pyArrayOut[arrayID]);

            // Keep a ref alive while we copy outside the GIL.
            Py_INCREF(pyArrayOut[arrayID]);
            outLen = static_cast<int>(PyArray_SIZE(npArrayOut[arrayID]));
            outPtr = reinterpret_cast<T*>(PyArray_DATA(npArrayOut[arrayID]));
        }

        wf_log("compute: copy NumPy output → host");
        outputVectors[arrayID].resize(outLen);
        std::memcpy(outputVectors[arrayID].data(),
                    outPtr,
                    static_cast<size_t>(outLen) * sizeof(T));

        {
            GILGuard g;
            Py_DECREF(pyArrayOut[arrayID]); // drop temporary hold
        }

        wf_log("compute: done array " + std::to_string(arrayID));

        // Optional debug printing: use host buffers (no GIL needed).
        out.verbose(CALL_INFO, 2, 0, "CrossSim MVM on array %u:\n", arrayID);

        const T* inputData  = hostInput[arrayID].data();
        const T* matrixData = hostMatrix[arrayID].data();
        const T* outputData = outputVectors[arrayID].data();

        for (uint32_t col = 0; col < inputArraySize; col++) {
            printValue(inputData[col]);
        }
        out.verbose(CALL_INFO, 2, 0, "\n\n");

        for (uint32_t row = 0; row < outputArraySize; row++) {
            for (uint32_t col = 0; col < inputArraySize; col++) {
                printValue(matrixData[row * inputArraySize + col]);
            }
            out.verbose(CALL_INFO, 2, 0, "  ");
            printValue(outputData[row]);
            out.verbose(CALL_INFO, 2, 0, "\n");
        }
        out.verbose(CALL_INFO, 2, 0, "\n\n");
    }

    virtual SimTime_t getArrayLatency(uint32_t) override { return 1; }

    // Stay host-side to avoid GIL here; compute() will push to NumPy later.
    virtual void moveOutputToInput(uint32_t srcArrayID, uint32_t destArrayID) override {
        wf_log("moveOutputToInput: " + std::to_string(srcArrayID) + " → " + std::to_string(destArrayID));
        std::copy(outputVectors[srcArrayID].begin(),
                  outputVectors[srcArrayID].begin() + outputArraySize,
                  hostInput[destArrayID].begin());
    }

    // Return host-side vectors (no GIL needed).
    virtual void* getInputVector(uint32_t arrayID) override {
        wf_log("getInputVector: array " + std::to_string(arrayID));
        return static_cast<void*>(&hostInput[arrayID]);
    }
    virtual void* getOutputVector(uint32_t arrayID) override {
        wf_log("getOutputVector: array " + std::to_string(arrayID));
        return static_cast<void*>(&outputVectors[arrayID]);
    }

protected:
    std::string CrossSimJSON;

    // Python object references
    PyObject* crossSim                = nullptr;
    PyObject* paramsConstructor       = nullptr;
    PyObject* AnalogCoreConstructor   = nullptr;
    PyObject* crossSim_params         = nullptr;

    // Arrays of references
    PyObject**      pyMatrix          = nullptr;
    PyArrayObject** npMatrix          = nullptr;
    PyObject**      pyArrayIn         = nullptr;
    PyArrayObject** npArrayIn         = nullptr;
    PyObject**      pyArrayOut        = nullptr;
    PyArrayObject** npArrayOut        = nullptr;
    PyObject**      cores             = nullptr;
    PyObject**      setMatrixFunction = nullptr;
    PyObject**      computeMVM        = nullptr;

    // Host-side copies (fast, thread-friendly)
    std::vector<std::vector<T>> inputVectors;   // kept for API parity
    std::vector<std::vector<T>> outputVectors;  // results copied here after compute()
    std::vector<std::vector<T>> hostMatrix;     // staging for matrix
    std::vector<std::vector<T>> hostInput;      // staging for vector

    int getNumpyType() {
        if constexpr (std::is_same<T, int64_t>::value)      return NPY_INT64;
        else if constexpr (std::is_same<T, float>::value)   return NPY_FLOAT32;
        else { static_assert(!sizeof(T*), "Unsupported data type for CrossSimComputeArray."); }
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
    static std::thread::id& initThread() {
        static std::thread::id t;
        return t;
    }

    static void doPythonInitialization() {
        wf_log("python-init: Py_Initialize()");
        initThread() = std::this_thread::get_id();
        Py_Initialize();                 // main thread holds the GIL after this
        gil_log("initializing Python (main holds GIL)");
        wf_log("python-init: import_array()");
        import_array1();                  // NumPy C-API init (requires GIL)
        mainState() = PyEval_SaveThread();   // release GIL so workers can acquire it
        gil_log("main released GIL after init");
        wf_log("python-init: done");
    }

    void initializePython() {
        wf_log("initializePython: maybe init once");
        std::call_once(initFlag(), doPythonInitialization);
        instanceCount().fetch_add(1, std::memory_order_acq_rel);
        wf_log("initializePython: instance count incremented");
    }

    void finalizePython() {
        wf_log("finalizePython: maybe finalize");
        if (instanceCount().fetch_sub(1, std::memory_order_acq_rel) != 1) {
            wf_log("finalizePython: other instances remain");
            return;
        }

        // Only finalize from the thread that initialized Python.
        if (std::this_thread::get_id() != initThread()) {
            wf_log("finalizePython: skip Py_Finalize() (not init thread)");
            return;
        }

        if (mainState()) {
            PyEval_RestoreThread(mainState());
            gil_log("main reacquired GIL for finalize");
            mainState() = nullptr;
        }
        Py_Finalize();
        gil_log("Python finalized");
        wf_log("finalizePython: finalized");
    }
};

} // namespace Golem
} // namespace SST

#endif /* _CROSSSIMCOMPUTEARRAY_H */


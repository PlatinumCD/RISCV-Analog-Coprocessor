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
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _CROSSSIMCOMPUTEARRAY_H
#define _CROSSSIMCOMPUTEARRAY_H

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <sst/elements/golem/array/computeArray.h>
#include <Python.h>
#include "numpy/arrayobject.h"
#include <string>
#include <type_traits>
#include <iostream>
#include <mutex>
#include <atomic>
#include <vector>
#include <algorithm>

namespace SST {
namespace Golem {

// -------------------- Thread-Safety Layer --------------------
namespace {
    static std::mutex numpy_mutex;

    inline PyObject* safe_PyArray_SimpleNew(int nd, npy_intp* dims, int typenum) {
        std::lock_guard<std::mutex> lock(numpy_mutex);
        return PyArray_SimpleNew(nd, dims, typenum);
    }

    inline PyObject* safe_PyImport_ImportModule(const char* name) {
        std::lock_guard<std::mutex> lock(numpy_mutex);
        return PyImport_ImportModule(name);
    }

    inline PyObject* safe_PyObject_GetAttrString(PyObject* obj, const char* name) {
        std::lock_guard<std::mutex> lock(numpy_mutex);
        return PyObject_GetAttrString(obj, name);
    }

    template<typename... Args>
    inline PyObject* safe_PyObject_CallFunctionObjArgs(PyObject* callable, Args... args) {
        std::lock_guard<std::mutex> lock(numpy_mutex);
        return PyObject_CallFunctionObjArgs(callable, args..., NULL);
    }
} // anonymous namespace
// -------------------------------------------------------------

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
        for (uint32_t i = 0; i < numArrays; i++) {
            Py_XDECREF(pyMatrix[i]);
            Py_XDECREF(pyArrayIn[i]);
            Py_XDECREF(pyArrayOut[i]);
            Py_XDECREF(cores[i]);
            Py_XDECREF(setMatrixFunction[i]);
            Py_XDECREF(computeMVM[i]);
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

        Py_XDECREF(crossSim);
        Py_XDECREF(paramsConstructor);
        Py_XDECREF(AnalogCoreConstructor);
        Py_XDECREF(crossSim_params);
        finalizePython();
    }

    virtual void init(unsigned int phase) override {
        if (phase == 0) {
            uint64_t inputSize = inputArraySize;
            uint64_t outputSize = outputArraySize;

            int matrixNumDims = 2;
            npy_intp matrixDims[2] = {
                static_cast<npy_intp>(outputSize),
                static_cast<npy_intp>(inputSize)
            };

            int arrayInNumDims = 1;
            npy_intp arrayInDim[1] = { static_cast<npy_intp>(inputSize) };

            int arrayOutNumDims = 1;
            npy_intp arrayOutDim[1] = { static_cast<npy_intp>(outputSize) };

            int numpyType = getNumpyType();

            for (uint32_t i = 0; i < numArrays; i++) {
                pyMatrix[i] = safe_PyArray_SimpleNew(matrixNumDims, matrixDims, numpyType);
                npMatrix[i] = reinterpret_cast<PyArrayObject*>(pyMatrix[i]);

                pyArrayIn[i] = safe_PyArray_SimpleNew(arrayInNumDims, arrayInDim, numpyType);
                npArrayIn[i] = reinterpret_cast<PyArrayObject*>(pyArrayIn[i]);

                pyArrayOut[i] = safe_PyArray_SimpleNew(arrayOutNumDims, arrayOutDim, numpyType);
                npArrayOut[i] = reinterpret_cast<PyArrayObject*>(pyArrayOut[i]);
            }

            crossSim = safe_PyImport_ImportModule("simulator");
            if (!crossSim) { out.fatal(CALL_INFO, -1, "Import CrossSim failed\n"); PyErr_Print(); }

            paramsConstructor = safe_PyObject_GetAttrString(crossSim, "CrossSimParameters");
            if (!paramsConstructor) { out.fatal(CALL_INFO, -1, "Get CrossSimParameters failed\n"); PyErr_Print(); }

            AnalogCoreConstructor = safe_PyObject_GetAttrString(crossSim, "AnalogCore");
            if (!AnalogCoreConstructor) { out.fatal(CALL_INFO, -1, "Get AnalogCore failed\n"); PyErr_Print(); }

            if (CrossSimJSON.empty()) {
                crossSim_params = PyObject_CallFunction(paramsConstructor, NULL);
            } else {
                PyObject* fromJson = safe_PyObject_GetAttrString(paramsConstructor, "from_json");
                PyObject* jsonArgs = Py_BuildValue("(s)", CrossSimJSON.c_str());
                crossSim_params = PyObject_CallObject(fromJson, jsonArgs);
                Py_XDECREF(fromJson);
                Py_XDECREF(jsonArgs);
            }
            if (!crossSim_params) { out.fatal(CALL_INFO, -1, "CrossSimParameters call failed\n"); PyErr_Print(); }

            for (uint32_t i = 0; i < numArrays; i++) {
                cores[i] = safe_PyObject_CallFunctionObjArgs(AnalogCoreConstructor, pyMatrix[i], crossSim_params);
                if (!cores[i]) { out.fatal(CALL_INFO, -1, "AnalogCore failed\n"); PyErr_Print(); }

                setMatrixFunction[i] = safe_PyObject_GetAttrString(cores[i], "set_matrix");
                if (!setMatrixFunction[i]) { out.fatal(CALL_INFO, -1, "core.set_matrix failed\n"); PyErr_Print(); }

                computeMVM[i] = safe_PyObject_GetAttrString(cores[i], "matvec");
                if (!computeMVM[i]) { out.fatal(CALL_INFO, -1, "core.matvec failed\n"); PyErr_Print(); }
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
        T* data = reinterpret_cast<T*>(PyArray_DATA(npMatrix[arrayID]));
        data[index] = static_cast<T>(value);

        if (index == inputArraySize * outputArraySize - 1) {
            PyObject* status = safe_PyObject_CallFunctionObjArgs(setMatrixFunction[arrayID], npMatrix[arrayID]);
            if (!status) { out.fatal(CALL_INFO, -1, "Call to core.set_matrix failed\n"); PyErr_Print(); }
            Py_XDECREF(status);
        }
    }

    virtual void setVectorItem(int32_t arrayID, int32_t index, double value) override {
        T* data = reinterpret_cast<T*>(PyArray_DATA(npArrayIn[arrayID]));
        data[index] = static_cast<T>(value);
    }

    virtual void compute(uint32_t arrayID) override {
        pyArrayOut[arrayID] = safe_PyObject_CallFunctionObjArgs(computeMVM[arrayID], npArrayIn[arrayID]);
        if (!pyArrayOut[arrayID]) { out.fatal(CALL_INFO, -1, "Run MVM Call Failed\n"); PyErr_Print(); }
        npArrayOut[arrayID] = reinterpret_cast<PyArrayObject*>(pyArrayOut[arrayID]);

        int len = PyArray_SIZE(npArrayOut[arrayID]);
        outputVectors[arrayID].resize(len);
        T* outputData = reinterpret_cast<T*>(PyArray_DATA(npArrayOut[arrayID]));
        std::copy(outputData, outputData + len, outputVectors[arrayID].begin());
    }

    virtual SimTime_t getArrayLatency(uint32_t) override { return 1; }

    virtual void moveOutputToInput(uint32_t srcArrayID, uint32_t destArrayID) override {
        T* src = reinterpret_cast<T*>(PyArray_DATA(npArrayOut[srcArrayID]));
        T* dst = reinterpret_cast<T*>(PyArray_DATA(npArrayIn[destArrayID]));
        std::copy(src, src + outputArraySize, dst);
    }

    virtual void* getInputVector(uint32_t arrayID) override {
        T* data = reinterpret_cast<T*>(PyArray_DATA(npArrayIn[arrayID]));
        int len = PyArray_SIZE(npArrayIn[arrayID]);
        inputVectors[arrayID].resize(len);
        std::copy(data, data + len, inputVectors[arrayID].begin());
        return static_cast<void*>(&inputVectors[arrayID]);
    }

    virtual void* getOutputVector(uint32_t arrayID) override {
        return static_cast<void*>(&outputVectors[arrayID]);
    }

protected:
    std::string CrossSimJSON;

    PyObject* crossSim = nullptr;
    PyObject* paramsConstructor = nullptr;
    PyObject* AnalogCoreConstructor = nullptr;
    PyObject* crossSim_params = nullptr;

    PyObject** pyMatrix = nullptr;
    PyArrayObject** npMatrix = nullptr;
    PyObject** pyArrayIn = nullptr;
    PyArrayObject** npArrayIn = nullptr;
    PyObject** pyArrayOut = nullptr;
    PyArrayObject** npArrayOut = nullptr;
    PyObject** cores = nullptr;
    PyObject** setMatrixFunction = nullptr;
    PyObject** computeMVM = nullptr;

    std::vector<std::vector<T>> inputVectors;
    std::vector<std::vector<T>> outputVectors;

    int getNumpyType() {
        if constexpr (std::is_same<T, int64_t>::value) return NPY_INT64;
        else if constexpr (std::is_same<T, float>::value) return NPY_FLOAT32;
        else static_assert(!sizeof(T*), "Unsupported type for CrossSimComputeArray.");
    }

private:
    static std::atomic<int>& getInstanceCount() {
        static std::atomic<int> count{0};
        return count;
    }

    static std::once_flag& getInitFlag() {
        static std::once_flag flag;
        return flag;
    }

    static void doPythonInitialization() {
        Py_Initialize();
        import_array1();
    }

    void initializePython() {
        std::call_once(getInitFlag(), doPythonInitialization);
        ++getInstanceCount();
    }

    void finalizePython() {
        if (--getInstanceCount() == 0) {
            Py_Finalize();
        }
    }
};

} // namespace Golem
} // namespace SST

#endif /* _CROSSSIMCOMPUTEARRAY_H */


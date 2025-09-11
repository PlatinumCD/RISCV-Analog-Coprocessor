#ifndef _CROSSSIMCOMPUTEARRAY_H
#define _CROSSSIMCOMPUTEARRAY_H

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <sst/elements/golem/array/computeArray.h>
#include <Python.h>
#include "numpy/arrayobject.h"

#include <string>
#include <type_traits>
#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <future>
#include <unordered_map>

// ==================================================================
// Global Python Worker Infrastructure
// ==================================================================
namespace {
    static std::once_flag py_init_flag;
    static std::atomic<int> py_instance_count{0};

    static std::mutex queue_mutex;
    static std::condition_variable queue_cv;
    static bool worker_running = false;

    struct Task {
        std::function<void()> fn;
        std::promise<void> done;
    };
    static std::queue<std::shared_ptr<Task>> task_queue;
    static std::thread python_worker;

    struct PyState {
        std::vector<PyObject*> pyMatrix;
        std::vector<PyObject*> pyArrayIn;
        std::vector<PyObject*> pyArrayOut;
        std::vector<PyObject*> cores;
        std::vector<PyObject*> setMatrixFunction;
        std::vector<PyObject*> computeMVM;
        PyObject* paramsConstructor = nullptr;
        PyObject* AnalogCoreConstructor = nullptr;
        PyObject* crossSim_params = nullptr;
    };
    static std::unordered_map<const void*, PyState> stateMap;

    void pythonWorkerMain() {
        std::cerr << "[PythonWorker] Running tasks..." << std::endl;
        worker_running = true;
        while (worker_running) {
            std::shared_ptr<Task> task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                queue_cv.wait(lock, [] { return !task_queue.empty() || !worker_running; });
                if (!worker_running) break;
                task = std::move(task_queue.front());
                task_queue.pop();
            }
            try {
                task->fn();
            } catch (const std::exception& e) {
                std::cerr << "[PythonWorker] Exception: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[PythonWorker] Unknown exception" << std::endl;
            }
            task->done.set_value();
        }
    }

    void ensureWorker() {
        std::call_once(py_init_flag, [] {
            python_worker = std::thread(pythonWorkerMain);
        });
    }

    void enqueueBlocking(std::function<void()> fn) {
        auto task = std::make_shared<Task>();
        task->fn = std::move(fn);
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            task_queue.push(task);
        }
        queue_cv.notify_one();
        task->done.get_future().wait();
    }

    void shutdownWorker() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            worker_running = false;
        }
        queue_cv.notify_one();
        if (python_worker.joinable()) python_worker.join();
    }

    // Call these from main thread
    void globalPythonInit() {
        std::cerr << "[Main] Initializing Python + NumPy..." << std::endl;
        Py_Initialize();
        import_array1();
        if (PyRun_SimpleString("import numpy") != 0) {
            std::cerr << "[Main] ERROR: failed to import numpy\n";
            PyErr_Print();
        }
        if (PyRun_SimpleString("import simulator") != 0) {
            std::cerr << "[Main] ERROR: failed to import simulator\n";
            PyErr_Print();
        }
        std::cerr << "[Main] Python + NumPy + simulator ready." << std::endl;
    }

    void globalPythonFinalize() {
        std::cerr << "[Main] Finalizing Python..." << std::endl;
        Py_Finalize();
    }
}

// ==================================================================
// CrossSimComputeArray
// ==================================================================
namespace SST {
namespace Golem {

template<typename T>
class CrossSimComputeArray : public ComputeArray {
public:
    CrossSimComputeArray(ComponentId_t id, Params& params,
                         TimeConverter* tc,
                         Event::HandlerBase* handler)
        : ComputeArray(id, params, tc, handler)
    {
        ensureWorker();
        ++py_instance_count;
        CrossSimJSON = params.find<std::string>("CrossSimJSONParameters");

        selfLink = configureSelfLink("Self", tc,
            new Event::Handler2<CrossSimComputeArray,
                                &CrossSimComputeArray::handleSelfEvent>(this));
        selfLink->setDefaultTimeBase(latencyTC);

        inputVectors.resize(numArrays);
        outputVectors.resize(numArrays);
        for (uint32_t i = 0; i < numArrays; i++) {
            inputVectors[i].resize(inputArraySize, T());
            outputVectors[i].resize(outputArraySize, T());
        }

        enqueueBlocking([this] { stateMap[this] = PyState(); });
        std::cerr << "[CrossSimComputeArray] Created instance at " << this << std::endl;
    }

    ~CrossSimComputeArray() {
        enqueueBlocking([this] { stateMap.erase(this); });
        if (--py_instance_count == 0) {
            shutdownWorker();
        }
        std::cerr << "[CrossSimComputeArray] Destroyed instance at " << this << std::endl;
    }

    void init(unsigned int phase) override {
        if (phase == 0) {
            enqueueBlocking([this] {
                std::cerr << "[CrossSimComputeArray] init() phase 0 for " << this << std::endl;
                auto& st = stateMap[this];

                uint64_t inputSize = inputArraySize;
                uint64_t outputSize = outputArraySize;
                int numpyType = getNumpyType();

                for (uint32_t i = 0; i < numArrays; i++) {
                    npy_intp matrixDims[2] = { (npy_intp)outputSize, (npy_intp)inputSize };
                    npy_intp inDim[1] = { (npy_intp)inputSize };
                    npy_intp outDim[1] = { (npy_intp)outputSize };

                    st.pyMatrix.push_back(PyArray_SimpleNew(2, matrixDims, numpyType));
                    st.pyArrayIn.push_back(PyArray_SimpleNew(1, inDim, numpyType));
                    st.pyArrayOut.push_back(PyArray_SimpleNew(1, outDim, numpyType));
                }
                std::cerr << "[CrossSimComputeArray] NumPy arrays created" << std::endl;

                PyObject* modules = PyImport_GetModuleDict();
                PyObject* crossSim = PyDict_GetItemString(modules, "simulator");
                if (!crossSim) {
                    std::cerr << "[CrossSimComputeArray] ERROR: simulator not found\n";
                    PyErr_Print();
                    return;
                }

                st.paramsConstructor = PyObject_GetAttrString(crossSim, "CrossSimParameters");
                if (!st.paramsConstructor) {
                    std::cerr << "[CrossSimComputeArray] ERROR: CrossSimParameters not found\n";
                    PyErr_Print();
                    return;
                }
                st.AnalogCoreConstructor = PyObject_GetAttrString(crossSim, "AnalogCore");
                if (!st.AnalogCoreConstructor) {
                    std::cerr << "[CrossSimComputeArray] ERROR: AnalogCore not found\n";
                    PyErr_Print();
                    return;
                }

                if (CrossSimJSON.empty()) {
                    st.crossSim_params = PyObject_CallFunction(st.paramsConstructor, NULL);
                } else {
                    PyObject* fromJson = PyObject_GetAttrString(st.paramsConstructor, "from_json");
                    if (!fromJson) {
                        std::cerr << "[CrossSimComputeArray] ERROR: CrossSimParameters.from_json not found\n";
                        PyErr_Print();
                        return;
                    }
                    PyObject* jsonArgs = Py_BuildValue("(s)", CrossSimJSON.c_str());
                    st.crossSim_params = PyObject_CallObject(fromJson, jsonArgs);
                    Py_XDECREF(fromJson);
                    Py_XDECREF(jsonArgs);
                }
                if (!st.crossSim_params) {
                    std::cerr << "[CrossSimComputeArray] ERROR: crossSim_params construction failed\n";
                    PyErr_Print();
                    return;
                }

                for (uint32_t i = 0; i < numArrays; i++) {
                    PyObject* core = PyObject_CallFunctionObjArgs(
                        st.AnalogCoreConstructor, st.pyMatrix[i], st.crossSim_params, NULL);
                    if (!core) {
                        std::cerr << "[CrossSimComputeArray] ERROR: core creation failed\n";
                        PyErr_Print();
                        return;
                    }
                    st.cores.push_back(core);

                    PyObject* smf = PyObject_GetAttrString(core, "set_matrix");
                    if (!smf) {
                        std::cerr << "[CrossSimComputeArray] ERROR: set_matrix not found\n";
                        PyErr_Print();
                        return;
                    }
                    st.setMatrixFunction.push_back(smf);

                    PyObject* mvm = PyObject_GetAttrString(core, "matvec");
                    if (!mvm) {
                        std::cerr << "[CrossSimComputeArray] ERROR: matvec not found\n";
                        PyErr_Print();
                        return;
                    }
                    st.computeMVM.push_back(mvm);
                }
                std::cerr << "[CrossSimComputeArray] init() complete for " << this << std::endl;
            });
        }
    }

    void beginComputation(uint32_t arrayID) override {
        SimTime_t latency = getArrayLatency(arrayID);
        ArrayEvent* ev = new ArrayEvent(arrayID);
        selfLink->send(latency, ev);
    }

    void handleSelfEvent(Event* ev) override {
        ArrayEvent* aev = static_cast<ArrayEvent*>(ev);
        uint32_t arrayID = aev->getArrayID();
        compute(arrayID);
        (*tileHandler)(ev);
    }

    void setMatrixItem(int32_t arrayID, int32_t index, double value) override {
        enqueueBlocking([=] {
            auto& st = stateMap[this];
            if (arrayID >= st.pyMatrix.size()) {
                std::cerr << "[CrossSimComputeArray] ERROR: setMatrixItem invalid arrayID\n";
                return;
            }
            T* data = (T*)PyArray_DATA((PyArrayObject*)st.pyMatrix[arrayID]);
            data[index] = static_cast<T>(value);
            if (index == inputArraySize * outputArraySize - 1) {
                PyObject* status = PyObject_CallFunctionObjArgs(st.setMatrixFunction[arrayID],
                                                                st.pyMatrix[arrayID], NULL);
                if (!status) {
                    std::cerr << "[CrossSimComputeArray] ERROR: set_matrix call failed\n";
                    PyErr_Print();
                }
                Py_XDECREF(status);
            }
        });
    }

    void setVectorItem(int32_t arrayID, int32_t index, double value) override {
        inputVectors[arrayID][index] = static_cast<T>(value);
        enqueueBlocking([=] {
            auto& st = stateMap[this];
            if (arrayID >= st.pyArrayIn.size()) {
                std::cerr << "[CrossSimComputeArray] ERROR: setVectorItem invalid arrayID\n";
                return;
            }
            T* data = (T*)PyArray_DATA((PyArrayObject*)st.pyArrayIn[arrayID]);
            data[index] = static_cast<T>(value);
        });
    }

    void compute(uint32_t arrayID) override {
        enqueueBlocking([=] {
            auto& st = stateMap[this];
            if (arrayID >= st.computeMVM.size()) {
                std::cerr << "[CrossSimComputeArray] ERROR: compute invalid arrayID\n";
                return;
            }
            PyObject* result = PyObject_CallFunctionObjArgs(st.computeMVM[arrayID],
                                                            st.pyArrayIn[arrayID], NULL);
            if (!result) {
                std::cerr << "[CrossSimComputeArray] ERROR: compute matvec failed\n";
                PyErr_Print();
                return;
            }
            st.pyArrayOut[arrayID] = result;

            PyArrayObject* arr = (PyArrayObject*)result;
            int len = PyArray_SIZE(arr);
            outputVectors[arrayID].resize(len);
            T* outputData = (T*)PyArray_DATA(arr);
            std::copy(outputData, outputData + len, outputVectors[arrayID].begin());
            std::cerr << "[CrossSimComputeArray] compute() done for array " << arrayID << std::endl;
        });
    }

    void moveOutputToInput(uint32_t srcArrayID, uint32_t destArrayID) override {
        enqueueBlocking([=] {
            auto& st = stateMap[this];
            if (srcArrayID >= st.pyArrayOut.size() || destArrayID >= st.pyArrayIn.size()) {
                std::cerr << "[CrossSimComputeArray] ERROR: moveOutputToInput invalid IDs\n";
                return;
            }
            T* src = (T*)PyArray_DATA((PyArrayObject*)st.pyArrayOut[srcArrayID]);
            T* dst = (T*)PyArray_DATA((PyArrayObject*)st.pyArrayIn[destArrayID]);
            std::copy(src, src + outputArraySize, dst);
        });
    }

    SimTime_t getArrayLatency(uint32_t) override { return 1; }

    void* getInputVector(uint32_t arrayID) override {
        return &inputVectors[arrayID];
    }

    void* getOutputVector(uint32_t arrayID) override {
        return &outputVectors[arrayID];
    }

private:
    std::string CrossSimJSON;
    std::vector<std::vector<T>> inputVectors;
    std::vector<std::vector<T>> outputVectors;

    int getNumpyType() {
        if constexpr (std::is_same<T, int64_t>::value) return NPY_INT64;
        else if constexpr (std::is_same<T, float>::value) return NPY_FLOAT32;
        else static_assert(!sizeof(T*), "Unsupported type for CrossSimComputeArray.");
    }
};

} // namespace Golem
} // namespace SST

#endif /* _CROSSSIMCOMPUTEARRAY_H */


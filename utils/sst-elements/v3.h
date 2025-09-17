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

    struct Task {
        std::function<void()> fn;
        std::promise<void> done;
    };

    static std::once_flag py_once;
    static PyThreadState* main_tstate = nullptr;

    static std::thread worker;
    static std::mutex q_mtx;
    static std::condition_variable q_cv;
    static std::queue<std::shared_ptr<Task>> q;

    static std::atomic<bool> worker_running{false};
    static std::atomic<int> live_instances{0};

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

    // Bootstrap Python once on the main thread
    inline void bootstrapPythonOnce() {
        std::call_once(py_once, [] {
            std::cerr << "[PythonMain] Initializing Python..." << std::endl;
            Py_Initialize();
            PyEval_InitThreads();  // prepare GIL
            PyGILState_STATE g = PyGILState_Ensure();
            import_array1();
            if (PyRun_SimpleString("import numpy as _np") != 0) {
                std::cerr << "[PythonMain] ERROR: import numpy\n";
                PyErr_Print();
            }
            if (PyRun_SimpleString("import simulator as _simulator") != 0) {
                std::cerr << "[PythonMain] ERROR: import simulator\n";
                PyErr_Print();
            }
            PyGILState_Release(g);
            main_tstate = PyEval_SaveThread();  // release GIL
            std::cerr << "[PythonMain] Python ready." << std::endl;
        });
    }

    // Worker loop
    static void workerMain() {
        std::cerr << "[PythonWorker] Starting..." << std::endl;
        worker_running = true;
        std::cerr << "[PythonWorker] Running tasks..." << std::endl;

        while (true) {
            std::shared_ptr<Task> task;
            {
                std::unique_lock<std::mutex> lock(q_mtx);
                q_cv.wait(lock, [] { return !q.empty() || !worker_running.load(); });
                if (!worker_running && q.empty()) break;
                task = std::move(q.front());
                q.pop();
            }

            PyGILState_STATE g = PyGILState_Ensure();
            try { task->fn(); }
            catch (const std::exception& e) { std::cerr << "[PythonWorker] Exception: " << e.what() << std::endl; }
            catch (...) { std::cerr << "[PythonWorker] Unknown exception\n"; }
            PyGILState_Release(g);

            task->done.set_value();
        }

        // Clean finalize
        if (main_tstate) {
            PyEval_RestoreThread(main_tstate);
            Py_Finalize();
            main_tstate = nullptr;
        }
        std::cerr << "[PythonWorker] Stopped." << std::endl;
    }

    // Enqueue and wait
    inline void enqueueBlocking(std::function<void()> fn) {
        auto task = std::make_shared<Task>();
        task->fn = std::move(fn);
        {
            std::lock_guard<std::mutex> lock(q_mtx);
            q.push(task);
        }
        q_cv.notify_one();
        task->done.get_future().wait();
    }

    inline void ensureWorker() {
        bootstrapPythonOnce();
        static std::once_flag once;
        std::call_once(once, [] { worker = std::thread(workerMain); });
    }

    inline void stopWorkerIfIdle() {
        if (live_instances.load() == 0) {
            {
                std::lock_guard<std::mutex> lock(q_mtx);
                worker_running = false;
            }
            q_cv.notify_one();
            if (worker.joinable()) worker.join();
        }
    }

    inline void teardownPyState(PyState& st) {
        for (auto* o : st.pyMatrix)          Py_XDECREF(o);
        for (auto* o : st.pyArrayIn)         Py_XDECREF(o);
        for (auto* o : st.pyArrayOut)        Py_XDECREF(o);
        for (auto* o : st.cores)             Py_XDECREF(o);
        for (auto* o : st.setMatrixFunction) Py_XDECREF(o);
        for (auto* o : st.computeMVM)        Py_XDECREF(o);
        Py_XDECREF(st.crossSim_params);
        Py_XDECREF(st.paramsConstructor);
        Py_XDECREF(st.AnalogCoreConstructor);
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
        ++live_instances;
        CrossSimJSON = params.find<std::string>("CrossSimJSONParameters");
        selfLink = configureSelfLink("Self", tc,
            new Event::Handler2<CrossSimComputeArray,
                                &CrossSimComputeArray::handleSelfEvent>(this));
        selfLink->setDefaultTimeBase(latencyTC);

        inputVectors.resize(numArrays, std::vector<T>(inputArraySize, T()));
        outputVectors.resize(numArrays, std::vector<T>(outputArraySize, T()));

        enqueueBlocking([this]{ stateMap.emplace(this, PyState{}); });
        std::cerr << "[CrossSimComputeArray] Created instance at " << this << std::endl;
    }

    ~CrossSimComputeArray() override {
        enqueueBlocking([this] {
            auto it = stateMap.find(this);
            if (it != stateMap.end()) {
                teardownPyState(it->second);
                stateMap.erase(it);
            }
        });
        if (--live_instances == 0) stopWorkerIfIdle();
        std::cerr << "[CrossSimComputeArray] Destroyed instance at " << this << std::endl;
    }

    void init(unsigned int phase) override {
        if (phase != 0) return;
        enqueueBlocking([this] {
            auto& st = stateMap[this];
            uint64_t inSize = inputArraySize, outSize = outputArraySize;
            int npType = getNumpyType();

            for (uint32_t i = 0; i < numArrays; i++) {
                npy_intp md[2] = { (npy_intp)outSize, (npy_intp)inSize };
                npy_intp xd[1] = { (npy_intp)inSize };
                npy_intp yd[1] = { (npy_intp)outSize };
                st.pyMatrix.push_back(PyArray_SimpleNew(2, md, npType));
                st.pyArrayIn.push_back(PyArray_SimpleNew(1, xd, npType));
                st.pyArrayOut.push_back(PyArray_SimpleNew(1, yd, npType));
            }
            std::cerr << "[CrossSimComputeArray] NumPy arrays created\n";

            PyObject* sim = PyDict_GetItemString(PyImport_GetModuleDict(), "simulator");
            if (!sim) { std::cerr << "simulator not loaded\n"; PyErr_Print(); return; }

            st.paramsConstructor = PyObject_GetAttrString(sim, "CrossSimParameters");
            st.AnalogCoreConstructor = PyObject_GetAttrString(sim, "AnalogCore");
            if (!st.paramsConstructor || !st.AnalogCoreConstructor) { PyErr_Print(); return; }

            if (CrossSimJSON.empty())
                st.crossSim_params = PyObject_CallFunction(st.paramsConstructor, nullptr);
            else {
                PyObject* from_json = PyObject_GetAttrString(st.paramsConstructor, "from_json");
                PyObject* arg = Py_BuildValue("(s)", CrossSimJSON.c_str());
                st.crossSim_params = PyObject_CallObject(from_json, arg);
                Py_XDECREF(from_json); Py_XDECREF(arg);
            }
            if (!st.crossSim_params) { PyErr_Print(); return; }

            for (uint32_t i = 0; i < numArrays; i++) {
                PyObject* core = PyObject_CallFunctionObjArgs(
                    st.AnalogCoreConstructor, st.pyMatrix[i], st.crossSim_params, nullptr);
                if (!core) { PyErr_Print(); return; }
                st.cores.push_back(core);
                st.setMatrixFunction.push_back(PyObject_GetAttrString(core, "set_matrix"));
                st.computeMVM.push_back(PyObject_GetAttrString(core, "matvec"));
            }
            std::cerr << "[CrossSimComputeArray] init() complete\n";
        });
    }

    void beginComputation(uint32_t arrayID) override {
        ArrayEvent* ev = new ArrayEvent(arrayID);
        selfLink->send(getArrayLatency(arrayID), ev);
    }

    void handleSelfEvent(Event* ev) override {
        compute(static_cast<ArrayEvent*>(ev)->getArrayID());
        (*tileHandler)(ev);
    }

    void setMatrixItem(int32_t arrayID, int32_t index, double v) override {
        enqueueBlocking([=] {
            auto& st = stateMap[this];
            auto* arr = (PyArrayObject*)st.pyMatrix[arrayID];
            ((T*)PyArray_DATA(arr))[index] = static_cast<T>(v);
            if (index == inputArraySize*outputArraySize - 1) {
                PyObject* ok = PyObject_CallFunctionObjArgs(st.setMatrixFunction[arrayID],
                                                            st.pyMatrix[arrayID], nullptr);
                if (!ok) PyErr_Print();
                Py_XDECREF(ok);
            }
        });
    }

    void setVectorItem(int32_t arrayID, int32_t index, double v) override {
        inputVectors[arrayID][index] = static_cast<T>(v);
        enqueueBlocking([=] {
            auto& st = stateMap[this];
            auto* arr = (PyArrayObject*)st.pyArrayIn[arrayID];
            ((T*)PyArray_DATA(arr))[index] = static_cast<T>(v);
        });
    }

    void compute(uint32_t arrayID) override {
        enqueueBlocking([=] {
            auto& st = stateMap[this];
            PyObject* r = PyObject_CallFunctionObjArgs(st.computeMVM[arrayID],
                                                       st.pyArrayIn[arrayID], nullptr);
            if (!r) { PyErr_Print(); return; }
            Py_XDECREF(st.pyArrayOut[arrayID]);
            st.pyArrayOut[arrayID] = r;
            auto* arr = (PyArrayObject*)r;
            int len = PyArray_SIZE(arr);
            outputVectors[arrayID].resize(len);
            std::copy((T*)PyArray_DATA(arr), (T*)PyArray_DATA(arr)+len,
                      outputVectors[arrayID].begin());
        });
    }

    void moveOutputToInput(uint32_t s, uint32_t d) override {
        enqueueBlocking([=] {
            auto& st = stateMap[this];
            T* src = (T*)PyArray_DATA((PyArrayObject*)st.pyArrayOut[s]);
            T* dst = (T*)PyArray_DATA((PyArrayObject*)st.pyArrayIn[d]);
            std::copy(src, src+outputArraySize, dst);
        });
    }

    SimTime_t getArrayLatency(uint32_t) override { return 1; }
    void* getInputVector(uint32_t id) override { return &inputVectors[id]; }
    void* getOutputVector(uint32_t id) override { return &outputVectors[id]; }

private:
    std::string CrossSimJSON;
    std::vector<std::vector<T>> inputVectors, outputVectors;

    int getNumpyType() {
        if constexpr (std::is_same<T, int64_t>::value) return NPY_INT64;
        else if constexpr (std::is_same<T, float>::value) return NPY_FLOAT32;
        else static_assert(!sizeof(T*), "Unsupported type");
    }
};

} // Golem
} // SST

#endif

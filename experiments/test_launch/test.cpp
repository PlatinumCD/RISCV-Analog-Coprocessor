#include <cstdio>
#include <sched.h>
#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <omp.h>

extern "C" {
uint64_t mvm_set(const void* A, int tile_id);
uint64_t mvm_load(const void* x, int tile_id);
uint64_t mvm_exec(int tile_id);
uint64_t mvm_store(void* y, int tile_id);
}


void pin_thread_to_core(int core){
    cpu_set_t m; CPU_ZERO(&m); CPU_SET(core, &m);
    sched_setaffinity(0, sizeof(cpu_set_t), &m);
}


int main() {
    int N = 2;

    float* A = new float[N * N]();
    for (int i = 0; i < N * N; i++) {
        A[i] = i * 1.001f;
    }

    float* xa = new float[N]();
    for (int i = 0; i < N; i++) {
        xa[i] = i * 1.001f;
    }

    // allocate one N-length result vector per core
    float* ya[NUM_CORES];
    for (int c = 0; c < NUM_CORES; c++) {
        ya[c] = new float[N]();
    }

    omp_set_num_threads(NUM_CORES);
#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        pin_thread_to_core(tid);

        char buf[64];
        int len = snprintf(buf, sizeof buf,
                           "Thread %d running on CPU %d\n",
                           tid, sched_getcpu());
        write(STDOUT_FILENO, buf, len);

        float* x_local = new float[N];
        for (int i = 0; i < N; i++) {
            x_local[i] = xa[i] * tid;
        }

        // Each thread uses its own ya[tid] buffer
        mvm_set(A, 0);
        mvm_load(x_local, 0);   // load the scaled vector
        mvm_exec(0);
        mvm_store(ya[tid], 0);

        delete[] x_local;

#pragma omp barrier
    }

    // dump results
/*    for (int c = 0; c < NUM_CORES; c++) {
        printf("Result from core %d: ", c);
        for (int i = 0; i < N; i++) {
            printf("%f ", ya[c][i]);
        }
        printf("\n");
    }
*/

    // cleanup
    for (int c = 0; c < NUM_CORES; c++) delete[] ya[c];
    delete[] xa;
    delete[] A;
}

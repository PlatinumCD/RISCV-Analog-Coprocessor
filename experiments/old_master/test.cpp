#include <omp.h>
#include <sched.h>
#include <unistd.h>
#include <cstdio>
#include <cstdint>

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

  float* A = new float[N*N]();
  for (int i = 0; i < N*N; i++) { 
    A[i] = i * 1.001;
  }

  float* xa = new float[N]();
  float* xb = new float[N]();
  for (int i = 0; i < N; i++) {
    xa[i] = i * 1.001;
    xb[i] = (i+N) * 1.001 ;
  }

  float* ya = new float[N]();
  float* yb = new float[N]();

  int status;
  int tile_id;

omp_set_num_threads(NUM_CORES);
#pragma omp parallel
  {
    int tid = omp_get_thread_num();
    pin_thread_to_core(tid);
    printf("Thread %d running on CPU %d\n", tid, sched_getcpu());

  // ########### Tile 0 #############
  mvm_set(A, 0);  mvm_load(xa, 0);  mvm_exec(0); // mvm_store(ya, 0);
  
  // ########### Tile 1 #############
//  mvm_set(A, 0);  mvm_load(xb, 0);  mvm_exec(0);  mvm_store(yb, 0);
//#pragma omp barrier
  }
}


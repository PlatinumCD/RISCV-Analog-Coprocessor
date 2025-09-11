// cg_persistent_mvm.cpp
#include <omp.h>
#include <sched.h>
#include <unistd.h>
#include <cstring>
#include <cmath>
#include <iostream>

static inline void pin_thread_to_core(int core){
    cpu_set_t m; CPU_ZERO(&m); CPU_SET(core, &m);
    sched_setaffinity(0, sizeof(m), &m);
}

static inline float dot(const float* a, const float* b, int n){
    float s=0.0f; for(int i=0;i<n;++i) s+=a[i]*b[i]; return s;
}
static inline void axpy(float* y, const float* x, float a, int n){
    for(int i=0;i<n;++i) y[i]+=a*x[i];
}
static inline void xmy(float* y, const float* x, float a, int n){
    for(int i=0;i<n;++i) y[i]-=a*x[i];
}

static inline void fill_chunks_from_A(const float* A, float** chunks){
    const int N=1024, T=128, G=8;
    for(int k=0;k<64;++k){
        int tr=k/G, tc=k%G;
        const float* src0=A + tr*T*N + tc*T;
        float* dst=chunks[k];
        for(int r=0;r<T;++r) std::memcpy(dst + r*T, src0 + r*N, T*sizeof(float));
    }
}

// ---- persistent MVM: stage once, then reuse ----
static inline void stage_tiles_persistent(float** chunks){
    if (NUM_CORES * NUM_ARRAYS < 64) {
        std::cerr << "Error: NUM_CORES*NUM_ARRAYS must be >= 64 for persistent staging.\n";
        std::exit(1);
    }
    omp_set_num_threads(NUM_CORES);
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        pin_thread_to_core(tid);
        int nth = omp_get_num_threads();
        for(int k=tid; k<64; k+=nth){
            int arr = k / nth;                    // per-core array slot
            if (arr >= NUM_ARRAYS) continue;      // (shouldn’t happen if product>=64)
            int status;
            asm volatile("mvm.set %0, %1, %2"
                         : "=r"(status)
                         : "r"(chunks[k]), "r"(arr)
                         : "memory");
        }
        #pragma omp barrier
    }
}

static inline void apply_Ax_persistent(const float* x, float* y){
    constexpr int G=8, T=128, N=1024;
    // per-thread partial accumulation (avoid races; reduced serially)
    float* partials = new float[NUM_CORES * N]();
    #pragma omp parallel num_threads(NUM_CORES) default(none) shared(x,partials)
    {
        int tid = omp_get_thread_num();
        pin_thread_to_core(tid);
        float* part = partials + tid * N;
        alignas(64) float tmp[T];

        for(int k=tid; k<64; k+=NUM_CORES){
            int tr  = k / G;              // row tile
            int tc  = k % G;              // col tile
            int arr = k / NUM_CORES;      // same slot used at stage time

            int status;
            asm volatile("mvm.l %0, %1, %2"
                         : "=r"(status) : "r"(x + tc*T), "r"(arr) : "memory");
            asm volatile("mvm %0, %1, x0"
                         : "=r"(status) : "r"(arr));
            asm volatile("mvm.s %0, %1, %2"
                         : "=r"(status) : "r"(tmp), "r"(arr) : "memory");

            float* dst = part + tr*T;
            for(int i=0;i<T;++i) dst[i] += tmp[i];
        }
    }
    // serial reduction of partials -> y
    std::memset(y, 0, N*sizeof(float));
    for(int t=0;t<NUM_CORES;++t){
        const float* part = partials + t*1024;
        for(int i=0;i<1024;++i) y[i] += part[i];
    }
    delete[] partials;
}

int main(){
    const int n=1024;

    // Build SPD tridiagonal A
    float* A = new float[n*n];
    for(int i=0;i<n;++i)
        for(int j=0;j<n;++j)
            A[i*n+j] = (i==j)?2.0f : (std::abs(i-j)==1 ? -1.0f : 0.0f);

    // Vectors
    float *b=new float[n], *x=new float[n], *r=new float[n], *p=new float[n], *Ap=new float[n];
    for(int i=0;i<n;++i){ b[i]=1.0f; x[i]=0.0f; }

    // Tiles (8×8 of 128×128)
    float** chunks = new float*[64];
    for(int k=0;k<64;++k) chunks[k] = new float[128*128]();
    fill_chunks_from_A(A, chunks);

    // Stage once, persistent
    stage_tiles_persistent(chunks);

    // CG
    const float tol=1e-6f, tol2=tol*tol; const int maxit=1000;
    apply_Ax_persistent(x, Ap);                  // Ap = A*x
    for(int i=0;i<n;++i){ r[i]=b[i]-Ap[i]; p[i]=r[i]; }
    float rsold = dot(r,r,n);

    int k=0;
    while(k<maxit && rsold>tol2){
        apply_Ax_persistent(p, Ap);             // Ap = A*p
        float denom = dot(p,Ap,n);
        if (std::fabs(denom) < 1e-30f) break;

        float alpha = rsold/denom;
        axpy(x, p,  alpha, n);                  // x += alpha*p
        xmy (r, Ap, alpha, n);                  // r -= alpha*Ap

        float rsnew = dot(r,r,n);
        ++k;
        if(rsnew<=tol2){ rsold=rsnew; break; }

        float beta = rsnew/rsold;
        for(int i=0;i<n;++i) p[i] = r[i] + beta*p[i];
        rsold = rsnew;
    }

    std::cout << "CG iters: " << k << "\n";

    // cleanup
    for(int i=0;i<64;++i) delete[] chunks[i];
    delete[] chunks;
    delete[] A; delete[] b; delete[] x; delete[] r; delete[] p; delete[] Ap;
    return 0;
}


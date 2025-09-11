// cg_persistent_mvm.cpp
#include <omp.h>
#include <sched.h>
#include <unistd.h>
#include <cstring>
#include <cmath>
#include <cstdint>
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
        const float* src0=A + tr*T*N + tc*T;  // top-left of the (tr,tc) tile in A
        float* dst=chunks[k];
        for(int r=0;r<T;++r) std::memcpy(dst + r*T, src0 + r*N, T*sizeof(float));
    }
}

// CPU-only: compute y = A*x using 8×8 tiles (64 M×v ops), distributed over NUM_CORES
static inline void apply_Ax_tiled_cpu(float** chunks, const float* x, float* y){
    constexpr int G=8, T=128, N=1024;

    // per-thread partials to avoid races, reduced after the parallel loop
    float* partials = new float[NUM_CORES * N]();  // zero-init
    float* scratch  = new float[NUM_CORES * T]();  // per-thread tmp for tile result

    omp_set_num_threads(NUM_CORES);
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        pin_thread_to_core(tid);

        float* part = partials + tid * N;
        float* tmp  = scratch  + tid * T;

        // Each thread processes a subset of the 64 tiles
        for(int k = tid; k < 64; k += NUM_CORES){
            int tr = k / G;            // row tile index
            int tc = k % G;            // col tile index

            const float* tile = chunks[k];   // T×T block
            const float* xsub = x + tc*T;    // length-T slice

            // tmp = tile * xsub
            for(int r=0; r<T; ++r){
                float s = 0.0f;
                const float* row = tile + r*T;
                for(int c=0; c<T; ++c) s += row[c] * xsub[c];
                tmp[r] = s;
            }

            // accumulate into the (tr) block of the output
            float* dst = part + tr*T;
            for(int r=0; r<T; ++r) dst[r] += tmp[r];
        }
        #pragma omp barrier
    }

    // reduce partials -> y
    std::memset(y, 0, N*sizeof(float));
    for(int t=0; t<NUM_CORES; ++t){
        const float* part = partials + t*N;
        for(int i=0;i<N;++i) y[i] += part[i];
    }

    delete[] scratch;
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

    // CG
    const float tol=1e-3f, tol2=tol*tol;
    const int maxit=2;

    apply_Ax_tiled_cpu(chunks, x, Ap);              // Ap = A*x
    for(int i=0;i<n;++i){ r[i]=b[i]-Ap[i]; p[i]=r[i]; }
    float rsold = dot(r,r,n);

    int k=0;
    while(k<maxit && rsold>tol2){
        apply_Ax_tiled_cpu(chunks, p, Ap);          // Ap = A*p
        float denom = dot(p,Ap,n);
        if (std::fabs(denom) < 1e-30f) break;

        float alpha = rsold/denom;
        axpy(x, p,  alpha, n);                      // x += alpha*p
        xmy (r, Ap, alpha, n);                      // r -= alpha*Ap

        float rsnew = dot(r,r,n);
        ++k;
        if(rsnew<=tol2){ rsold=rsnew; break; }

        float beta = rsnew/rsold;
        for(int i=0;i<n;++i) p[i] = r[i] + beta*p[i];
        rsold = rsnew;
        std::cout << "rsold: " << rsold << std::endl;
    }

    std::cout << "Fin-rsold: " << rsold << std::endl;
    std::cout << "CG iters: " << k << "\n";

    // cleanup
    for(int i=0;i<64;++i) delete[] chunks[i];
    delete[] chunks;
    delete[] A; delete[] b; delete[] x; delete[] r; delete[] p; delete[] Ap;
    return 0;
}


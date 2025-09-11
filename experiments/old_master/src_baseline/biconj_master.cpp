#include <omp.h>
#include <sched.h>
#include <unistd.h>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <algorithm>

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
    constexpr int N=1024, T=128, G=8; // 64 tiles, row-major 8x8
    for(int k=0;k<64;++k){
        int tr=k/G, tc=k%G;
        const float* src0=A + tr*T*N + tc*T;
        float* dst=chunks[k];
        for(int r=0;r<T;++r) std::memcpy(dst + r*T, src0 + r*N, T*sizeof(float));
    }
}

// y = A*x using 8x8 T×T tiles (CPU-only), distributed across NUM_CORES
static inline void apply_Ax_tiled_cpu(float** chunks, const float* x, float* y){
    constexpr int G=8, T=128, N=1024;

    float* partials = new float[NUM_CORES * N](); // per-thread accumulators
    float* scratch  = new float[NUM_CORES * T](); // per-thread tile result

    omp_set_num_threads(NUM_CORES);
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        pin_thread_to_core(tid);

        float* part = partials + tid * N;
        float* tmp  = scratch  + tid * T;

        for(int k=tid; k<64; k+=NUM_CORES){
            int tr = k / G;               // row tile
            int tc = k % G;               // col tile

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
    constexpr int n=1024;

    // Mildly non-symmetric, diagonally-dominant tridiagonal with slight skew
    float* A = new float[n*n];
    for(int i=0;i<n;++i){
        for(int j=0;j<n;++j){
            if(i==j) A[i*n+j]=2.0f;
            else if(std::abs(i-j)==1) A[i*n+j]=-1.0f;
            else A[i*n+j]=0.0f;
        }
    }
    for(int i=0;i+1<n;++i) A[i*n + (i+1)] -= 0.2f;

    // Vectors
    float *b=new float[n], *x=new float[n];
    for(int i=0;i<n;++i){ b[i]=1.0f; x[i]=0.0f; }

    // Tiles (8×8 of 128×128)
    float** chunks = new float*[64];
    for(int k=0;k<64;++k) chunks[k] = new float[128*128]();
    fill_chunks_from_A(A, chunks);

    // BiCGSTAB data
    float *r=new float[n], *rhat=new float[n], *p=new float[n], *v=new float[n], *s=new float[n], *t=new float[n];

    // r = b - A*x
    apply_Ax_tiled_cpu(chunks, x, v);
    for(int i=0;i<n;++i) r[i]=b[i]-v[i];
    for(int i=0;i<n;++i) rhat[i]=r[i];
    for(int i=0;i<n;++i){ p[i]=0.0f; v[i]=0.0f; }

    float rho_old=1.0f, alpha=1.0f, omega=1.0f;
    const float bnorm = std::max(1.0f, std::sqrt(dot(b,b,n)));
    const float tol = 1e-3f, tol_abs = tol * bnorm, eps = 1e-30f;
    float r_norm = std::sqrt(dot(r,r,n));

    const int maxit=2;
    int k=0;
    while(k<maxit){
        float rho_new = dot(rhat, r, n);
        if (std::fabs(rho_new) < eps) { k=-1; break; }

        float beta = (rho_new/(rho_old+eps))*(alpha/(omega+eps));
        for(int i=0;i<n;++i) p[i] = r[i] + beta*(p[i] - omega*v[i]);

        apply_Ax_tiled_cpu(chunks, p, v);                 // v = A*p
        float rhat_v = dot(rhat, v, n);
        if (std::fabs(rhat_v) < eps) { k=-1; break; }
        alpha = rho_new / rhat_v;

        for(int i=0;i<n;++i) s[i] = r[i] - alpha*v[i];
        float s_norm = std::sqrt(dot(s,s,n));
        if (s_norm <= tol_abs){
            axpy(x, p, alpha, n);
            ++k; break;
        }

        apply_Ax_tiled_cpu(chunks, s, t);                 // t = A*s
        float tt = dot(t,t,n);
        if (std::fabs(tt) < eps) { k=-1; break; }
        omega = dot(t,s,n)/tt;
        if (std::fabs(omega) < eps) { k=-1; break; }

        for(int i=0;i<n;++i) x[i] += alpha*p[i] + omega*s[i];
        for(int i=0;i<n;++i) r[i]  = s[i] - omega*t[i];

        r_norm = std::sqrt(dot(r,r,n));
        ++k;
        if (r_norm <= tol_abs) break;

        rho_old = rho_new;
        std::cout << "r_norm: " << r_norm << std::endl;
    }

    std::cout << "Fin-r_norm: " << r_norm << std::endl;
    std::cout << "BiCGSTAB iters: " << k << "\n";

    // cleanup
    for(int i=0;i<64;++i) delete[] chunks[i];
    delete[] chunks;
    delete[] A; delete[] b; delete[] x;
    delete[] r; delete[] rhat; delete[] p; delete[] v; delete[] s; delete[] t;
    return 0;
}


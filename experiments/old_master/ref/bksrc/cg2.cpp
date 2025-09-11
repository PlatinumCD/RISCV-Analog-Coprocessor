#include <omp.h>
#include <iostream>
#include <cstring> // memcpy
#include <iomanip>
#include <cmath>

static inline void pin_thread_to_core(int core){
    cpu_set_t m; CPU_ZERO(&m); CPU_SET(core, &m);
    sched_setaffinity(0, sizeof(m), &m);
}


static inline float dot(const float* a, const float* b, int n) {
    float s = 0.0;
    for (int i = 0; i < n; ++i) {
        s += a[i] * b[i];
    }
    return s;
}

static inline void axpy(float* y, const float* x, float a, int n) {
    for (int i = 0; i < n; ++i) {
        y[i] += a * x[i];
    }
}

static inline void copy(float* dst, const float* src, int n) {
    for (int i = 0; i < n; ++i) {
        dst[i] = src[i];
    }
}

static inline void scale(float* x, float a, int n) {
    for (int i = 0; i < n; ++i) {
        x[i] *= a;
    }
}


void fill_chunks_from_A(const float* A, float** chunks) {
    const int N = 1024, T = 128, G = 8; // N = matrix dim, T = tile dim, G = tiles per dim
    for (int k = 0; k < G*G; ++k) {
        int tr = k / G;                // tile row [0..7]
        int tc = k % G;                // tile col [0..7]
        const float* src0 = A + tr*T*N + tc*T;
        float* dst = chunks[k];
        for (int r = 0; r < T; ++r)    // copy one tile row at a time
            std::memcpy(dst + r*T, src0 + r*N, T * sizeof(float));
    }
}

int main() {
    const int n = 1024;

    float* A = new float[n * n];
    float* b = new float[n];
    float* x = new float[n];

    // Build SPD tridiagonal: 2 on diag, -1 on first off-diagonals
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i == j) {
                A[i * n + j] = 2.0;
            } else if (std::abs(i - j) == 1) {
                A[i * n + j] = -1.0;
            } else {
                A[i * n + j] = 0.0;
            }
        }
    }

    for (int i = 0; i < n; ++i) {
        b[i] = 1.0;
        x[i] = 0.0; // initial guess
    }


    float** chunks = new float*[64];
    for (int i = 0; i < 64; i++) {
        chunks[i] = new float[128 * 128]();
    }

    fill_chunks_from_A(A, chunks);

#pragma omp parallel
{
    int tid = omp_get_thread_num();
    int nth = omp_get_num_threads();

    pin_thread_to_core(tid);

    for (int i = tid; i < NUM_ARRAYS; i += nth) {
	int status_flag;
	float* tile_ptr = chunks[i];
	asm volatile (
	    "mvm.set %0, %1, %2"
	  : "=r" (status_flag)
	  : "r"(tile_ptr), "r"(i)
	  : "memory"
	);
    }
    #pragma omp barrier
}
    
    float tol = 1e-6
    matit = 1000;

    float* r  = new float[n];
    float* p  = new float[n];
    float* Ap = new float[n];

    int status_flag;
    int tile_id = 0;
    asm volatile (
        "mvm.l %0, %1, %2"
      : "=r" (status_flag)
      : "r"(x), "r"(tile_id)
      : "memory"
    );

    asm volatile (
        "mvm %0, %1, x0"
        : "=r" (status_flag)
        : "r"(tile_id)
    );

    asm volatile (
        "mvm.s %0, %1, %2"
        : "=r" (status_flag)
        : "r"(Ap), "r"(tile_id)
        : "memory"
    );

    for (int i = 0; i < n; ++i) {
        r[i] = b[i] - Ap[i];
        p[i] = r[i];
    }

    const float tol2 = tol * tol;
    float rsold = dot(r, r, n);

    int k = 0;
    while (k < maxit && rsold > tol2) {

        asm volatile (
            "mvm.l %0, %1, %2"
          : "=r" (status_flag)
          : "r"(p), "r"(tile_id)
          : "memory"
        );
    
        asm volatile (
            "mvm %0, %1, x0"
            : "=r" (status_flag)
            : "r"(tile_id)
        );
    
        asm volatile (
            "mvm.s %0, %1, %2"
            : "=r" (status_flag)
            : "r"(Ap), "r"(tile_id)
            : "memory"
        ); 

        float denom = dot(p, Ap, n);
        if (std::fabs(denom) < 1e-30) {
            break; // numerical safeguard
        }

        float alpha = rsold / denom;

        // x = x + alpha * p
        for (int i = 0; i < n; ++i) {
            x[i] += alpha * p[i];
        }
        // r = r - alpha * Ap
        for (int i = 0; i < n; ++i) {
            r[i] -= alpha * Ap[i];
        }

        float rsnew = dot(r, r, n);
        ++k;

        if (rsnew <= tol2) {
            rsold = rsnew;
            break;
        }

        float beta = rsnew / rsold;

        // p = r + beta * p
        for (int i = 0; i < n; ++i) {
            p[i] = r[i] + beta * p[i];
        }

        rsold = rsnew;
    }

    delete[] r;
    delete[] p;
    delete[] Ap;
    delete[] A;
    delete[] b;
    delete[] x;
    for (int i = 0; i < 64; i++) delete[] chunks[i];
    delete[] chunks;
    return 0;
}


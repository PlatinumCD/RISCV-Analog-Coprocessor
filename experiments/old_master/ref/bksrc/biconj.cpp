#include <iostream>
#include <iomanip>
#include <cmath>

static inline float dot(const float* a, const float* b, int n) {
    float s = 0.0;
    for (int i = 0; i < n; ++i) s += a[i] * b[i];
    return s;
}

#if NUM_ARRAYS == 0
static inline void matvec(const float* A, const float* x, float* y, int n) {
    for (int i = 0; i < n; ++i) {
        const float* Ai = A + i * n;
        float s = 0.0;
        for (int j = 0; j < n; ++j) s += Ai[j] * x[j];
        y[i] = s;
    }
}
#endif

static inline float nrm2(const float* x, int n) {
    return std::sqrt(dot(x, x, n));
}

// BiCGSTAB: solves Ax=b for general nonsymmetric A.
// Returns iterations used (or -1 on breakdown).

#if NUM_ARRAYS == 0
int bicgstab(const float* A, const float* b, float* x, int n,
             float tol = 1e-8, int maxit = -1) {
#else
int bicgstab(const float* b, float* x, int n,
             float tol = 1e-8, int maxit = -1) {

#endif
    if (maxit < 0) maxit = 2 * n;

    float* r     = new float[n];
    float* rhat  = new float[n];
    float* p     = new float[n];
    float* v     = new float[n];
    float* s     = new float[n];
    float* t     = new float[n];

    // r = b - A x
#if NUM_ARRAYS == 0
    matvec(A, x, v, n);
#else
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
       : "r"(v), "r"(tile_id)
       : "memory"
   ); 
#endif

    for (int i = 0; i < n; ++i) r[i] = b[i] - v[i];

    // rhat = r (fixed shadow)
    for (int i = 0; i < n; ++i) rhat[i] = r[i];

    for (int i = 0; i < n; ++i) { p[i] = 0.0; v[i] = 0.0; }

    float rho_old = 1.0;
    float alpha   = 1.0;
    float omega   = 1.0;

    const float bnorm = std::max(1.0f, nrm2(b, n));
    const float tol_abs = tol * bnorm;

    int k = 0;
    while (k < maxit) {
        float rho_new = dot(rhat, r, n);
        if (std::fabs(rho_new) < 1e-30) { k = -1; break; }

        float beta = (rho_new / rho_old) * (alpha / omega);

        // p = r + beta * (p - omega * v)
        for (int i = 0; i < n; ++i) p[i] = r[i] + beta * (p[i] - omega * v[i]);

#if NUM_ARRAYS == 0
        matvec(A, p, v, n);
#else
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
            : "r"(v), "r"(tile_id)
            : "memory"
        ); 

#endif

        float rhat_v = dot(rhat, v, n);
        if (std::fabs(rhat_v) < 1e-30) { k = -1; break; }

        alpha = rho_new / rhat_v;

        // s = r - alpha * v
        for (int i = 0; i < n; ++i) s[i] = r[i] - alpha * v[i];

        if (nrm2(s, n) <= tol_abs) {
            // x = x + alpha * p
            for (int i = 0; i < n; ++i) x[i] += alpha * p[i];
            ++k;
            break;
        }

#if NUM_ARRAYS == 0
        matvec(A, s, t, n);
#else
        asm volatile (
            "mvm.l %0, %1, %2"
          : "=r" (status_flag)
          : "r"(s), "r"(tile_id)
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
            : "r"(t), "r"(tile_id)
            : "memory"
        ); 
#endif

        float tt = dot(t, t, n);
        if (std::fabs(tt) < 1e-30) { k = -1; break; }

        omega = dot(t, s, n) / tt;
        if (std::fabs(omega) < 1e-30) { k = -1; break; }

        // x = x + alpha * p + omega * s
        for (int i = 0; i < n; ++i) x[i] += alpha * p[i] + omega * s[i];

        // r = s - omega * t
        for (int i = 0; i < n; ++i) r[i] = s[i] - omega * t[i];

        if (nrm2(r, n) <= tol_abs) { ++k; break; }

        rho_old = rho_new;
        ++k;
    }

    delete[] r; delete[] rhat; delete[] p; delete[] v; delete[] s; delete[] t;
    return k;
}

int main() {
    const int n = 1024;
    float* A = new float[n * n];
    float* b = new float[n];
    float* x = new float[n];

    // Example nonsymmetric but convergent matrix: diagonally dominant + slight skew
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i == j)       A[i*n + j] = 2.0;
            else if (std::abs(i - j) == 1) A[i*n + j] = -1.0;
            else               A[i*n + j] = 0.0;
        }
    }
    // Introduce a little nonsymmetry
    for (int i = 0; i+1 < n; ++i) A[i*n + (i+1)] -= 0.2;

    for (int i = 0; i < n; ++i) {
        b[i] = 1.0;
        x[i] = 0.0; // initial guess
    }
 
#if NUM_ARRAYS == 0
    int iters = bicgstab(A, b, x, n, 1e-6, 1000);
#else
    // Perform MVM
    int status_flag;
    int tile_id = 0;
    asm volatile (
        "mvm.set %0, %1, %2"
      : "=r" (status_flag)
      : "r"(A), "r"(tile_id)
      : "memory"
    );
 
    int iters = bicgstab(b, x, n, 1e-6, 1000);
#endif

    std::cout << "iters = " << iters << "\nsolution:\n";
    std::cout << std::setprecision(6);
    for (int i = 0; i < n; ++i) {
        std::cout << x[i] << (i + 1 == n ? '\n' : ' ');
    }

    delete[] A; delete[] b; delete[] x;
    return 0;
}

#include <iostream>
#include <iomanip>
#include <cmath>

static inline float dot(const float* a, const float* b, int n) {
    float s = 0.0;
    for (int i = 0; i < n; ++i) s += a[i] * b[i];
    return s;
}

static inline float nrm2(const float* x, int n) {
    return std::sqrt(dot(x, x, n));
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
 
    // Perform MVM
    int status_flag;
    int tile_id = 0;
    asm volatile (
        "mvm.set %0, %1, %2"
      : "=r" (status_flag)
      : "r"(A), "r"(tile_id)
      : "memory"
    );

    float tol = 1e-3;
    int maxit = 1000;

    float* r     = new float[n];
    float* rhat  = new float[n];
    float* p     = new float[n];
    float* v     = new float[n];
    float* s     = new float[n];
    float* t     = new float[n];

    // r = b - A x
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

    for (int i = 0; i < n; ++i) r[i] = b[i] - v[i];
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
        for (int i = 0; i < n; ++i) p[i] = r[i] + beta * (p[i] - omega * v[i]);

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

        float rhat_v = dot(rhat, v, n);
        if (std::fabs(rhat_v) < 1e-30) { k = -1; break; }
        alpha = rho_new / rhat_v;
        for (int i = 0; i < n; ++i) s[i] = r[i] - alpha * v[i];
        if (nrm2(s, n) <= tol_abs) {
            for (int i = 0; i < n; ++i) x[i] += alpha * p[i];
            ++k;
            break;
        }

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

        float tt = dot(t, t, n);
        if (std::fabs(tt) < 1e-30) { k = -1; break; }
        omega = dot(t, s, n) / tt;
        if (std::fabs(omega) < 1e-30) { k = -1; break; }

        for (int i = 0; i < n; ++i) x[i] += alpha * p[i] + omega * s[i];
        for (int i = 0; i < n; ++i) r[i] = s[i] - omega * t[i];
        if (nrm2(r, n) <= tol_abs) { ++k; break; }
        rho_old = rho_new;
        ++k;
    }

    std::cout << "iters = " << iters << "\nsolution:\n";
    std::cout << std::setprecision(6);
    for (int i = 0; i < n; ++i) {
        std::cout << x[i] << (i + 1 == n ? '\n' : ' ');
    }

    delete[] A; delete[] b; delete[] x;
    delete[] r; delete[] rhat; delete[] p; delete[] v; delete[] s; delete[] t;

    return 0;
}

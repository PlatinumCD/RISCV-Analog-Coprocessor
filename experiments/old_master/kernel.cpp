#include <cstdint>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
uint64_t mvm_set(const void* A, int tile_id) {
    uint64_t status;
    uintptr_t a = (uintptr_t)A;
    asm volatile("mvm.set %0, %1, %2"
                 : "=r"(status)
                 : "r"(a), "r"(tile_id)
                 : "memory","cc");
    return status;
}

uint64_t mvm_load(const void* x, int tile_id) {
    uint64_t status;
    uintptr_t xp = (uintptr_t)x;
    asm volatile("mvm.l %0, %1, %2"
                 : "=r"(status)
                 : "r"(xp), "r"(tile_id)
                 : "memory","cc");
    return status;
}

uint64_t mvm_exec(int tile_id) {
    uint64_t status;
    asm volatile("mvm %0, %1, x0"
                 : "=r"(status)
                 : "r"(tile_id)
                 : "memory","cc");
    return status;
}

uint64_t mvm_store(void* y, int tile_id) {
    uint64_t status;
    uintptr_t yp = (uintptr_t)y;
    asm volatile("mvm.s %0, %1, %2"
                 : "=r"(status)
                 : "r"(yp), "r"(tile_id)
                 : "memory","cc");
    return status;
}
#ifdef __cplusplus
}
#endif

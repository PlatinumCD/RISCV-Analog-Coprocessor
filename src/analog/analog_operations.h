#include <cstdint>

/*

void mvm_set_matrix(AnalogMatrix mat, uint32_t tile_id) {
    mat.quantize();
    void* data = static_cast<void *>mat.get_tile_mat();
    int completion_flag;
    __asm__ __volatile__ (
        "mvm.set %0, %1, %2"
        : "=r"(data),
        : "=r"(tile_id),
        : "=r"(completion_flag)
        : "memory"
    );
}

void mvm_load_vector(AnalogVector vec, uint32_t tile_id) {
    vec.quantize();
    void* data = static_cast<void *>vec.get_tile_vec();
    int completion_flag;
    __asm__ __volatile__ (
        "mvm.l %0, %1, %2"
        : "=r"(data),
        : "=r"(tile_id),
        : "=r"(completion_flag)
        : "memory"
    );
}    


    __asm__ __volatile__ ("mvm     x0, x0, x0");
    __asm__ __volatile__ ("mvm.s   x0, x0, x0");
    __asm__ __volatile__ ("mvm.mv  x0, x0, x0");
*/

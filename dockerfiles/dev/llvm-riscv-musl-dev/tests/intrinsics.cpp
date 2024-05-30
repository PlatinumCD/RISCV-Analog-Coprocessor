
int main() {

    int status_flag;
    int* data;
    int tile_id;
    int tile_id_a, tile_id_b;

    // mvm.set
    asm volatile (
        "mvm.set %0, %1, %2"
        : "=r" (status_flag)
        : "r"(data), "r"(tile_id)
        : "memory"
    );

    // mvm.l
    asm volatile (
        "mvm.l %0, %1, %2"
        : "=r" (status_flag)
        : "r"(data), "r"(tile_id)
        : "memory"
    );

    // mvm
    asm volatile (
        "mvm %0, %1, x0"
        : "=r" (status_flag)
        : "r"(tile_id)
    );

    // mvm.s
    asm volatile (
        "mvm.s %0, %1, %2"
        : "=r" (status_flag)
        : "r"(data), "r"(tile_id)
        : "memory"
    );

    // mvm.mv
    asm volatile (
        "mvm.mv %0, %1, %2"
        : "=r" (status_flag)
        : "r"(tile_id_a), "r"(tile_id_b)
    );

    // mvm.i
    asm volatile (
        "mvm.i %0, %1, x0"
        : "=r" (status_flag)
        : "r"(tile_id)
    );

    // mvm.o
    asm volatile (
        "mvm.o %0, %1, x0"
        : "=r" (status_flag)
        : "r"(tile_id)
    );
}

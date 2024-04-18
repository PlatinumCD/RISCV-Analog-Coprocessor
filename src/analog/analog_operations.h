/**
 * @file analog_operations.h
 * @brief This file defines several operations related to Analog data types.
 */

#include <cstdint>
#include "AnalogMatrix.h"
#include "AnalogVector.h"

/**
 * @brief Sets a matrix to a specified tile.
 * @param mat The matrix to set to the tile.
 * @param tile_id The ID of the tile to set the matrix.
 * @return The status flag indicating whether the operation was successful or not.
 */
template <class T>
uint32_t mvm_set_matrix(AnalogMatrix<T> &mat, uint32_t tile_id) {
    mat.quantize();
    void* data = static_cast<void *>(mat.get_device_mat());
    uint32_t status_flag;

    __asm__ __volatile__ (
        "mvm.set %0, %1, %2"
        : "=r"(status_flag)
        : "r"(data), "r"(tile_id)
        : "memory"
    );
    // Send scale_factor?
    return status_flag;
}

/**
 * @brief Loads a vector into a specified tile.
 * @param vec The vector to load into the tile.
 * @param tile_id The ID of the tile to load the vector.
 * @return The status flag indicating whether the operation was successful or not.
 */
template <class T>
uint32_t mvm_load_vector(AnalogVector<T> &vec, uint32_t tile_id) {
    vec.quantize();
    void* data = static_cast<void *>(vec.get_device_arr());
    uint32_t status_flag;

    __asm__ __volatile__ (
        "mvm.l %0, %1, %2"
        : "=r"(status_flag)
        : "r"(data), "r"(tile_id)
        : "memory"
    );
    // Send scale_factor?
    return status_flag;
}    

/**
 * @brief Performs a computation on a specified tile.
 * @param tile_id The ID of the tile on which to perform the computation.
 * @return The status flag indicating whether the operation was successful or not.
 */
uint32_t mvm_compute(uint32_t tile_id) {
    uint32_t status_flag;

    __asm__ __volatile__ (
        "mvm %0, %1, x0"
        : "=r"(status_flag)
        : "r"(tile_id)
    );

    return status_flag;
}

/**
 * @brief Stores a vector into a specified tile.
 * @param vec The vector to store into the tile.
 * @param tile_id The ID of the tile to store the vector.
 * @return The status flag indicating whether the operation was successful or not.
 */
template <class T>
uint32_t mvm_store_vector(AnalogVector<T> &vec, uint32_t tile_id) {
    void* data = static_cast<void *>(vec.get_device_arr());
    uint32_t status_flag;

    __asm__ __volatile__ (
        "mvm.s %0, %1, %2"
        : "=r"(data), "=r"(status_flag)
        : "r"(tile_id)
        : "memory"
    );
    // Recieve scale_factor?
//    vec.dequantize();
    return status_flag;
}

/**
 * @brief Moving the output vector from a tile to the input of another tile.
 * @param tile_id_a The ID of the source tile which has completed an mvm operation.
 * @param tile_id_b The ID of the target tile.
 * @return The status flag indicating whether the operation was successful or not.
 */
uint32_t mvm_move_vector(uint32_t tile_id_a, uint32_t tile_id_b) {
    uint32_t status_flag;

    __asm__ __volatile__ (
        "mvm.mv  %0, %1, %2"
        : "=r"(status_flag)
        : "r"(tile_id_a), "r"(tile_id_b)
    );

    return status_flag;
}

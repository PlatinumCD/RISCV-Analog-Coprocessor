/**
 * @file analog_operations.h
 * @brief This file defines several operations related to Analog data types.
 */

#ifndef ANALOG_OPERATIONS_H
#define ANALOG_OPERATIONS_H

#include <cstdint>
#include "AnalogMatrix.h"
#include "AnalogVector.h"
#include "AnalogContext.h"

/**
 * @brief Sets a matrix to a specified tile.
 * @param ctx The analog context managing the scales.
 * @param mat The matrix to set to the tile.
 * @param tile_id The ID of the tile to set the matrix.
 * @return The status flag indicating whether the operation was successful or not.
 */
template <class T>
uint32_t mvm_set_matrix(AnalogContext &ctx, AnalogMatrix<T> &mat, uint32_t tile_id) {
    mat.quantize(); // Quantize the matrix
    float scale = mat.get_scale(); // Get the matrix scale
    ctx.set_matrix_scale(scale, tile_id); // Set the matrix scale in the context

    T* data = mat.get_device_mat(); // Get the pointer to the device matrix data
    uint32_t status_flag;
    asm volatile (
        "mvm.set %0, %1, %2"
        : "=r" (status_flag)
        : "r"(data), "r"(tile_id)
        : "memory"
    );

    return status_flag;
}

/**
 * @brief Loads a vector into a specified tile.
 * @param ctx The analog context managing the scales.
 * @param vec The vector to load into the tile.
 * @param tile_id The ID of the tile to load the vector.
 * @return The status flag indicating whether the operation was successful or not.
 */
template <class T>
uint32_t mvm_load_vector(AnalogContext &ctx, AnalogVector<T> &vec, uint32_t tile_id) {
    vec.quantize(); // Quantize the vector
    float scale = vec.get_scale(); // Get the vector scale
    ctx.set_vector_scale(scale, tile_id); // Set the vector scale in the context

    T* data = vec.get_device_arr(); // Get the pointer to the device vector data
    uint32_t status_flag;

    asm volatile (
        "mvm.l %0, %1, %2"
        : "=r" (status_flag)
        : "r"(data), "r"(tile_id)
        : "memory"
    );

    return status_flag;
}

/**
 * @brief Performs a computation on a specified tile.
 * @param ctx The analog context managing the scales.
 * @param tile_id The ID of the tile on which to perform the computation.
 * @return The status flag indicating whether the operation was successful or not.
 */
uint32_t mvm_compute(AnalogContext &ctx, uint32_t tile_id) {
    uint32_t status_flag;

    asm volatile (
        "mvm %0, %1, x0"
        : "=r" (status_flag)
        : "r"(tile_id)
    );

    ctx.compute_scale(tile_id); // Compute the output scale
    return status_flag;
}

/**
 * @brief Stores a vector from a specified tile.
 * @param ctx The analog context managing the scales.
 * @param vec The vector to store from the tile.
 * @param tile_id The ID of the tile to store the vector from.
 * @return The status flag indicating whether the operation was successful or not.
 */
template <class T>
uint32_t mvm_store_vector(AnalogContext &ctx, AnalogVector<T> &vec, uint32_t tile_id) {
    float scale = ctx.get_scale(tile_id); // Get the output scale for dequantization
    T* data = vec.get_device_arr(); // Get the pointer to the device vector data
    uint32_t status_flag;

    asm volatile (
        "mvm.s %0, %1, %2"
        : "=r" (status_flag)
        : "r"(data), "r"(tile_id)
        : "memory"
    );

    vec.dequantize(scale); // Dequantize the vector based on the scale
    return status_flag;
}

/**
 * @brief Moves the output vector from a tile to the input of another tile.
 * @param tile_id_a The ID of the source tile which has completed an MVM operation.
 * @param tile_id_b The ID of the target tile.
 * @return The status flag indicating whether the operation was successful or not.
 */
uint32_t mvm_move_vector(uint32_t tile_id_a, uint32_t tile_id_b) {
    uint32_t status_flag;

    asm volatile (
        "mvm.mv %0, %1, %2"
        : "=r" (status_flag)
        : "r"(tile_id_a), "r"(tile_id_b)
    );

    return status_flag;
}

#endif // ANALOG_OPERATIONS_H

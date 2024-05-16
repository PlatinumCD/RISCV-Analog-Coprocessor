/**
 * @file AnalogContext.h
 * @brief This file contains the declaration and implementation of the AnalogContext class.
 */

#ifndef ANALOG_CONTEXT_H
#define ANALOG_CONTEXT_H

#include <iostream>
#include <cstdint>

// Forward declarations to avoid include dependencies
template <class T>
class AnalogMatrix;

template <class T>
class AnalogVector;

/**
 * @class AnalogContext
 * @brief The AnalogContext class keeps track of array scale factors.
 */
class AnalogContext {
public:
    /**
     * @brief Constructor of the AnalogContext class.
     * @param num_arrays The number of arrays for which scales are maintained.
     */
    AnalogContext(uint32_t num_arrays)
        : num_arrays(num_arrays),
          matrix_scales(new float[num_arrays]),
          vector_scales(new float[num_arrays]),
          output_scales(new float[num_arrays]) {
        // Initialize each element to 1.0
        for (uint32_t i = 0; i < num_arrays; ++i) {
            matrix_scales[i] = 1.0f;
            vector_scales[i] = 1.0f;
            output_scales[i] = 1.0f;
        }
    }

    /**
     * @brief Destructor of the AnalogContext class.
     */
    ~AnalogContext() {
        delete[] matrix_scales;
        delete[] vector_scales;
        delete[] output_scales;
    }

    /**
     * @brief Sets a specific value in the matrix scale factors array.
     * @param value The value to set.
     * @param index The index in the array.
     */
    void set_matrix_scale(float value, uint32_t index) {
        if (index < num_arrays) {
            matrix_scales[index] = value;
        }
    }

    /**
     * @brief Sets a specific value in the vector scale factors array.
     * @param value The value to set.
     * @param index The index in the array.
     */
    void set_vector_scale(float value, uint32_t index) {
        if (index < num_arrays) {
            vector_scales[index] = value;
        }
    }

    /**
     * @brief Compute the output vector scale factor.
     * @param index The index in the array.
     */
    void compute_scale(uint32_t index) {
        if (index < num_arrays) {
            output_scales[index] = matrix_scales[index] * vector_scales[index];
        }
    }

    /**
     * @brief Gets a specific value from the output scale factors array.
     * @param index The index in the array.
     * @return The value at the specified index.
     */
    float get_scale(uint32_t index) const {
        if (index < num_arrays) {
            return output_scales[index];
        }
        return 0.0f;
    }

    /**
     * @brief Prints the values at each index for both arrays.
     */
    void print() const {
        std::cout << "Matrix Scale Factors:" << std::endl;
        for (uint32_t i = 0; i < num_arrays; ++i) {
            std::cout << "Index " << i << ": " << matrix_scales[i] << std::endl;
        }

        std::cout << "Vector Scale Factors:" << std::endl;
        for (uint32_t i = 0; i < num_arrays; ++i) {
            std::cout << "Index " << i << ": " << vector_scales[i] << std::endl;
        }

        std::cout << "Output Scale Factors:" << std::endl;
        for (uint32_t i = 0; i < num_arrays; ++i) {
            std::cout << "Index " << i << ": " << output_scales[i] << std::endl;
        }
    }

private:
    uint32_t num_arrays; ///< Number of arrays
    float* matrix_scales;  ///< Array of matrix scale factors
    float* vector_scales;  ///< Array of vector scale factors
    float* output_scales;  ///< Array of output scale factors
};

#endif // ANALOG_CONTEXT_H

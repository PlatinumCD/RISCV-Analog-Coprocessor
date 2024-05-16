/**
 * @file AnalogVector.h
 * @brief This file contains the declaration and implementation of the AnalogVector class.
 */

#ifndef ANALOG_VECTOR_H
#define ANALOG_VECTOR_H

#include <cmath>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include "AnalogDataType.h"

/**
 * @class AnalogVector
 * @brief The AnalogVector class represents a vector interface that is compatible with MVM analog intrinsic calls.
 * @tparam T Data type of the elements in the vector.
 */
template <class T>
class AnalogVector : public AnalogDataType<T> {
public:
    /**
     * @brief Constructor of the AnalogVector class without an array.
     * @param length Length of the host array.
     */
    AnalogVector(uint32_t length)
        : host_arr(new float[length]()),
          host_length(length),
          device_arr(new T[DEVICE_COLS]()),
          device_length(DEVICE_COLS),
          owns_host_arr(true) {}

    /**
     * @brief Constructor of the AnalogVector class using an array.
     * @param arr Pointer to the host array.
     * @param length Length of the host array.
     */
    AnalogVector(float* arr, uint32_t length)
        : host_arr(arr),
          host_length(length),
          device_arr(new T[DEVICE_COLS]()),
          device_length(DEVICE_COLS),
          owns_host_arr(false) {}

    /**
     * @brief Destructor to clean up allocated device array memory.
     */
    ~AnalogVector() {
        if (owns_host_arr) {
            delete[] host_arr;
        }
        delete[] device_arr;
    }

    /**
     * @brief Allocates memory for the device array and then quantizes the input array to the range of type T by normalizing all values to the maximum absolute value.
     */
    void quantize() {
        // Identify the scaling factor
        float scale_factor = 0.0f;
        for (uint32_t i = 0; i < host_length; i++) {

            // TODO
            // fabs doesn't work on Vanadis?
            float tmp_val = (host_arr[i] < 0) ? -host_arr[i] : host_arr[i];
            if (tmp_val > scale_factor) {
                scale_factor = tmp_val;
            }
        }
        this->set_scale_factor(scale_factor);

        // Scale vector values to fit the type T limits
        T max_type_limit = this->get_max_type_limit();
        for (uint32_t i = 0; i < host_length; i++) {
            device_arr[i] = static_cast<T>(std::llround(static_cast<double>(host_arr[i]) / scale_factor * max_type_limit));
        }
    }

    /**
     * @brief Dequantizes the device array using the provided scale factor.
     * @param scale The scale factor used for dequantization.
     */
    void dequantize(float scale) {
        for (uint32_t i = 0; i < host_length; i++) {
            host_arr[i] = device_arr[i] * scale;
        }
    }

    /**
     * @brief Returns the quantized device array.
     * @return Pointer to the quantized device array.
     */
    T* get_device_arr() {
        return device_arr;
    }

    /**
     * @brief Prints the properties and content of the host and device arrays if they have been quantized.
     */
    void print() {
        if (device_arr == nullptr) {
            std::cout << "Vector not quantized." << std::endl;
            return;
        }

        std::cout << "Host Array Length: " << host_length << std::endl;
        std::cout << "\t";
        for (uint32_t i = 0; i < host_length; i++) {
            std::cout << host_arr[i] << " ";
        }
        std::cout << std::endl;

        std::cout << "Device Array Length: " << device_length << std::endl;
        std::cout << "Device Array:" << std::endl;
        std::cout << "\t";
        for (uint32_t i = 0; i < device_length; i++) {
            std::cout << std::setw(4) << static_cast<int>(device_arr[i]) << " ";
        }
        std::cout << std::endl;
    }

private:
    float* host_arr;        ///< Pointer to the host array
    uint32_t host_length;   ///< Length of the host array
    T* device_arr;          ///< Pointer to the quantized device array
    uint32_t device_length; ///< Length of the quantized device array
    bool owns_host_arr;     ///< Whether this object owns and should delete the host array
};

#endif // ANALOG_VECTOR_H

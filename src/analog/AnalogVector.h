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
 * @brief The AnalogVector class represents a vector interface that is compatible with MVM analog intrinsic calls 
 */
template <class T>
class AnalogVector : AnalogDataType<T> {
public:

    /**
     * @brief Constructor of the AnalogVector class without an array
     */
    AnalogVector(uint32_t length) :
        host_arr(nullptr),
        host_length(length), 
        device_arr(nullptr),
        device_length(DEVICE_COLS)
    {}

    /**
     * @brief Constructor of the AnalogVector class using an array 
     */
    AnalogVector(float* arr, uint32_t length) :
        host_arr(arr),
        host_length(length), 
        device_arr(nullptr),
        device_length(DEVICE_COLS)
    {}

    /**
     * @brief Allocates memory for the device array and then quantizes the input array to the range of type T by normalizing all values to the maximum absolute value.
     */
    void quantize() {
        // Allocate space for the device array
        device_arr = new T[device_length]();

        // Identify scale factor
        float scale_factor = 0.0f;
        for (uint32_t i = 0; i < host_length; i++) {
            if (fabs(host_arr[i]) > scale_factor) {
                scale_factor = fabs(host_arr[i]);
            }
        }
        this->set_scale_factor(scale_factor);

        // Scale vector values to fit type T limits
        T max_type_limit = this->get_max_type_limit();
        for (uint32_t i = 0; i < host_length; i++) {
            device_arr[i] = std::round(host_arr[i] / scale_factor * max_type_limit);
        }
    }

    /**
     * @brief Returns the quantized device array.
     */
    T** get_device_arr() {
        return device_arr;
    }

    /**
     * @brief Prints the properties and content of the device array if it has been quantized. 
     */
    void print() {

        if (device_arr == nullptr) {
            std::cout << "Vector not quantized." << std::endl;
            return;
        }

        std::cout << "Device Array Length: " << device_length << std::endl;
        std::cout << "Host Array Length: " << host_length << std::endl;
        std::cout << "Device Array:" << std::endl;
        std::cout << "\t";
        for (uint32_t i = 0; i < host_length+2; i++) {
            std::cout << std::setw(4) << static_cast<int>(device_arr[i]) << " ";
        }
        std::cout << "..." << std::endl;
        std::cout << std::endl;
    }

private:
    float* host_arr;
    uint32_t host_length;
    T* device_arr;
    uint32_t device_length;
};

#endif

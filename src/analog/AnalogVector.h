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
     * @brief Constructor of the AnalogVector class 
     */
    AnalogVector(float* arr, uint32_t length) :
        arr_length(TILE_COLS), 
        in_length(length), 
        in_arr(arr),
        tile_arr(nullptr)
    {}

    /**
     * @brief Allocates memory for the tile array and then quantizes the input array to the range of type T by normalizing all values to the maximum absolute value.
     */
    void quantize() {
        // Allocate space for the tile array
        tile_arr = new T[arr_length]();

        // Identify scale factor
        float scale_factor = 0.0f;
        for (uint32_t i = 0; i < in_length; i++) {
            if (fabs(in_arr[i]) > scale_factor) {
                scale_factor = fabs(in_arr[i]);
            }
        }
        this->set_scale_factor(scale_factor);

        // Scale vector values to fit type T limits
        T max_type_limit = this->get_max_type_limit();
        for (uint32_t i = 0; i < in_length; i++) {
            tile_arr[i] = std::round(in_arr[i] / scale_factor * max_type_limit);
        }
    }

    /**
     * @brief Returns the quantized tile matrix
     */
    T** get_tile_arr() {
        return tile_arr;
    }

    /**
     * @brief Prints the properties and content of the tile array if it has been quantized. 
     */
    void print() {

        if (tile_arr == nullptr) {
            std::cout << "Vector not quantized." << std::endl;
            return;
        }

        std::cout << "Array Length: " << arr_length << std::endl;
        std::cout << "Input Size: " << in_length << std::endl;
        std::cout << "Tile Array:" << std::endl;
        std::cout << "\t";
        for (uint32_t i = 0; i < in_length+2; i++) {
            std::cout << std::setw(4) << static_cast<int>(tile_arr[i]) << " ";
        }
        std::cout << "..." << std::endl;
        std::cout << std::endl;
    }

private:
    uint32_t arr_length;
    uint32_t in_length;
    float* in_arr;
    T* tile_arr;
};

#endif

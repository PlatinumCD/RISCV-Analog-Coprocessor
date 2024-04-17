/**
 * @file AnalogMatrix.h
 * @brief This file contains the declaration and implementation of the AnalogMatrix class.
 */

#ifndef ANALOG_MATRIX_H
#define ANALOG_MATRIX_H

#include <cmath>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <limits>

#include "AnalogDataType.h"

/**
 * @class AnalogMatrix
 * @brief The AnalogMatrix class represents a matrix interface that is compatible with MVM analog intrinsic calls
 */
template <class T>
class AnalogMatrix : private AnalogDataType<T> {
public:

    /**
     * @brief Constructor of the AnalogMatrix class 
     */
    AnalogMatrix(float** mat, uint32_t rows, uint32_t cols) :
        host_mat(mat),
        host_rows(rows), 
        host_cols(cols), 
        device_mat(nullptr),
        device_rows(DEVICE_ROWS),
        device_cols(DEVICE_COLS)
    {}

    /**
     * @brief Allocates memory for the device matrix and then quantizes the host matrix to the range of type T by normalizing all values to the maximum absolute value.
     */
    void quantize() {
        // Allocate space for device matrix
        device_mat = new T*[device_rows];
        for (uint32_t i = 0; i < device_rows; i++) {
            device_mat[i] = new T[device_cols]();
        }

        // Identify scale factor 
        float scale_factor = 0.0f;
        for (uint32_t i = 0; i < host_rows; i++) {
            for (uint32_t j = 0; j < host_cols; j++) {
                if (fabs(host_mat[i][j]) > scale_factor) {
                    scale_factor = fabs(host_mat[i][j]);
                }
            }
        }
        this->set_scale_factor(scale_factor);

        // Scale matrix values to fit type T limits
        T max_type_limit = this->get_max_type_limit();
        for (uint32_t i = 0; i < host_rows; i++) {
            for (uint32_t j = 0; j < host_cols; j++) {
                device_mat[i][j] = std::round(host_mat[i][j] / scale_factor * max_type_limit);
            }
        }
    }

    /**
     * @brief Returns the quantized device matrix
     */
    T** get_device_mat() {
        return device_mat;
    }

    /**
     * @brief Prints the properties and content of the device matrix if it has been quantized. 
     */
    void print() {

        if (device_mat == nullptr) {
            std::cout << "Matrix not quantized." << std::endl;
            return;
        }

        std::cout << "Tile Size: " << device_rows << "x" << device_cols << std::endl;
        std::cout << "Input Size: " << host_rows << "x" << host_cols << std::endl;
        std::cout << "Tile Matrix:" << std::endl;
        for (uint32_t i = 0; i < host_rows+2; i++) {
            std::cout << "\t";
            for (uint32_t j = 0; j < host_cols+2; j++) {
                std::cout << std::setw(4) << static_cast<int>(device_mat[i][j]) << " ";
            }
            std::cout << "..." << std::endl;
        }
        for (uint32_t i = 0; i < 2; i++) {
            std::cout << "\t";
            for (uint32_t j = 0; j < host_cols+2; j++) {
                std::cout << std::setw(5) << ". ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

private:
    float** host_mat;
    uint32_t host_rows, host_cols;
    T** device_mat;
    uint32_t device_rows, device_cols;
};

#endif

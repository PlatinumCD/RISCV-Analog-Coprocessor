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
 * @brief The AnalogMatrix class represents a matrix interface compatible with MVM analog intrinsic calls.
 * @tparam T Data type of the elements in the matrix.
 */
template <class T>
class AnalogMatrix : public AnalogDataType<T> {
public:
    /**
     * @brief Constructor for the AnalogMatrix class.
     * @param mat 2D array representing the host matrix.
     * @param rows Number of rows in the host matrix.
     * @param cols Number of columns in the host matrix.
     */
    AnalogMatrix(float** mat, uint32_t rows, uint32_t cols)
        : host_mat(mat), host_rows(rows), host_cols(cols),
          device_mat(new T[DEVICE_ROWS * DEVICE_COLS]()), device_rows(DEVICE_ROWS), device_cols(DEVICE_COLS) {
        std::fill(device_mat, device_mat + device_rows * device_cols, 0);
    }

    /**
     * @brief Destructor to clean up allocated device matrix memory.
     */
    ~AnalogMatrix() {
        delete[] device_mat;
    }

    /**
     * @brief Allocates memory for the device matrix and quantizes the host matrix to fit within the range of type T.
     */
    void quantize() {
        // Identify the scaling factor
        float scale_factor = 0.0f;
        for (uint32_t i = 0; i < host_rows; i++) {
            for (uint32_t j = 0; j < host_cols; j++) {

                // TODO
                // fabs doesn't work on Vanadis?
                float tmp_val = (host_mat[i][j] < 0) ? -host_mat[i][j] : host_mat[i][j];
                if (tmp_val > scale_factor) {
                    scale_factor = tmp_val;
                }
            }
        }
        this->set_scale_factor(scale_factor);

        // Scale matrix values to fit within type T limits
        T max_type_limit = this->get_max_type_limit();
        for (uint32_t i = 0; i < host_rows; i++) {
            for (uint32_t j = 0; j < host_cols; j++) {
                uint32_t device_index = i * device_cols + j;
                device_mat[device_index] = static_cast<T>(std::llround(static_cast<double>(host_mat[i][j]) / scale_factor * max_type_limit));
            }
        }
    }

    /**
     * @brief Returns the quantized device matrix.
     * @return Pointer to the quantized device matrix.
     */
    T* get_device_mat() {
        return device_mat;
    }

    /**
     * @brief Prints the properties and content of the device matrix, if it has been quantized.
     */
    void print() {
        if (device_mat == nullptr) {
            std::cout << "Matrix not quantized." << std::endl;
            return;
        }

        std::cout << "Host Size: " << host_rows << "x" << host_cols << std::endl;
        std::cout << "Host Matrix: " << std::endl;
        for (uint32_t i = 0; i < host_rows; i++) {
            std::cout << "\t";
            for (uint32_t j = 0; j < host_cols; j++) {
                std::cout << std::setw(4) << host_mat[i][j] << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "Max type limit: " << static_cast<int64_t>(this->get_max_type_limit()) << std::endl;
        std::cout << "Device Size: " << device_rows << "x" << device_cols << std::endl;
        std::cout << "Device Matrix:" << std::endl;
        for (uint32_t i = 0; i < device_rows; i++) {
            std::cout << "\t";
            for (uint32_t j = 0; j < device_cols; j++) {
                std::cout << std::setw(4) << static_cast<int64_t>(device_mat[i * device_cols + j]) << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

private:
    float** host_mat; ///< Pointer to the host matrix
    uint32_t host_rows; ///< Number of rows in the host matrix
    uint32_t host_cols; ///< Number of columns in the host matrix
    T* device_mat; ///< Pointer to the quantized device matrix
    uint32_t device_rows; ///< Number of rows in the device matrix
    uint32_t device_cols; ///< Number of columns in the device matrix
};

#endif // ANALOG_MATRIX_H

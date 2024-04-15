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
        tile_rows(TILE_ROWS), 
        tile_cols(TILE_COLS), 
        in_rows(rows), 
        in_cols(cols), 
        in_mat(mat),
        tile_mat(nullptr)
    {}

    /**
     * @brief Allocates memory for the tile matrix and then quantizes the input matrix to the range of type T by normalizing all values to the maximum absolute value.
     */
    void quantize() {
        // Allocate space for tile matrix
        tile_mat = new T*[tile_rows];
        for (uint32_t i = 0; i < tile_rows; i++) {
            tile_mat[i] = new T[tile_cols]();
        }

        // Identify scale factor 
        float scale_factor = 0.0f;
        for (uint32_t i = 0; i < in_rows; i++) {
            for (uint32_t j = 0; j < in_cols; j++) {
                if (fabs(in_mat[i][j]) > scale_factor) {
                    scale_factor = fabs(in_mat[i][j]);
                }
            }
        }
        this->set_scale_factor(scale_factor);

        // Scale matrix values to fit type T limits
        T max_type_limit = this->get_max_type_limit();
        for (uint32_t i = 0; i < in_rows; i++) {
            for (uint32_t j = 0; j < in_cols; j++) {
                tile_mat[i][j] = std::round(in_mat[i][j] / scale_factor * max_type_limit);
            }
        }
    }

    /**
     * @brief Returns the quantized tile matrix
     */
    T** get_tile_mat() {
        return tile_mat;
    }

    /**
     * @brief Prints the properties and content of the tile matrix if it has been quantized. 
     */
    void print() {

        if (tile_mat == nullptr) {
            std::cout << "Matrix not quantized." << std::endl;
            return;
        }

        std::cout << "Tile Size: " << tile_rows << "x" << tile_cols << std::endl;
        std::cout << "Input Size: " << in_rows << "x" << in_cols << std::endl;
        std::cout << "Tile Matrix:" << std::endl;
        for (uint32_t i = 0; i < in_rows+2; i++) {
            std::cout << "\t";
            for (uint32_t j = 0; j < in_cols+2; j++) {
                std::cout << std::setw(4) << static_cast<int>(tile_mat[i][j]) << " ";
            }
            std::cout << "..." << std::endl;
        }
        for (uint32_t i = 0; i < 2; i++) {
            std::cout << "\t";
            for (uint32_t j = 0; j < in_cols+2; j++) {
                std::cout << std::setw(5) << ". ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

private:
    uint32_t tile_rows, tile_cols;
    uint32_t in_rows, in_cols;
    float** in_mat;
    T** tile_mat;
};

#endif

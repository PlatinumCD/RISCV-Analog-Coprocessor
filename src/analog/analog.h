/**
 * @file Analog.h
 * @brief This file includes the necessary headers for usage of Analog Matrix and Vector classes.
 *
 * It also defines the constants TILE_ROWS and TILE_COLS which represent the default size of the 
 * tile matrix and tile vector respectively.
 */

#ifndef ANALOG_H
#define ANALOG_H

/**
 * @brief Default row size for tile matrix and vectors in analog computations.
 */
#define TILE_ROWS 512

/**
 * @brief Default column size for tile matrix and vectors in analog computations.
 */
#define TILE_COLS 1024

#include "AnalogMatrix.h"
#include "AnalogVector.h"

// TODO: Define intrinsic for getting details about the tiles
// TODO: Define intrinsic for scale factor updating?

#endif

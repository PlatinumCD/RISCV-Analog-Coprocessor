/**
 * @file analog.h
 * @brief This file includes the necessary headers for usage of Analog Matrix and Vector classes.
 *
 * It also defines the constants DEVICE_ROWS and DEVICE_COLS which represent the default size of the
 * device matrix and device vector respectively.
 */

#ifndef ANALOG_H
#define ANALOG_H

// Defining the default sizes for device matrix and vectors
#define DEVICE_ROWS 5 ///< Default row size for device matrix and vectors in analog computations.
#define DEVICE_COLS 6 ///< Default column size for device matrix and vectors in analog computations.

// Include the necessary headers for Analog Matrix, Vector, Context, and Operations
#include "AnalogMatrix.h"
#include "AnalogVector.h"
#include "AnalogContext.h"
#include "analog_operations.h"

// Placeholder for future intrinsic definitions for getting details about the tiles
// TODO: Define intrinsic for getting details about the tiles

#endif // ANALOG_H

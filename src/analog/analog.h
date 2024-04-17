/**
 * @file Analog.h
 * @brief This file includes the necessary headers for usage of Analog Matrix and Vector classes.
 *
 * It also defines the constants DEVICE_ROWS and DEVICE_COLS which represent the default size of the 
 * device matrix and device vector respectively.
 */

#ifndef ANALOG_H
#define ANALOG_H

/**
 * @brief Default row size for device matrix and vectors in analog computations.
 */
#define DEVICE_ROWS 512

/**
 * @brief Default column size for device matrix and vectors in analog computations.
 */
#define DEVICE_COLS 1024

#include "AnalogMatrix.h"
#include "AnalogVector.h"
#include "analog_operations.h"

// TODO: Define intrinsic for getting details about the tiles
// TODO: Define intrinsic for scale factor updating?

#endif

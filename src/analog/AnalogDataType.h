/**
 * @file AnalogDataType.h
 * @brief This file contains the declaration of the AnalogDataType class.
 */

#ifndef ANALOG_DATATYPE_H
#define ANALOG_DATATYPE_H

#include <limits>

/**
 * @class AnalogDataType
 * @brief The AnalogDataType is a base class for the data types that will be used in the analog computation models.
 *        It provides the functionality to set/get scale factor and to get the max limit of the type T.
 */
template <class T>
class AnalogDataType {
public:

    /**
     * @brief Constructor of the AnalogDataType class
     */
    AnalogDataType() :
        scale_factor(1.0f),
        max_type_limit(std::numeric_limits<T>::max())
    {}

    /**
     * @brief Sets the scale factor that will be used in the computations.
     */
    void set_scale_factor(float factor) {
        scale_factor = factor;
    }

    /**
     * @brief Returns the current scale factor
     */
    float get_scale_factor() {
        return scale_factor;
    }

    /**
     * @brief Returns the maximum possible value for the type T
     */
    T get_max_type_limit() {
        return max_type_limit;
    }

private:

    float scale_factor;
    T max_type_limit;
};

#endif

/**
 * @file AnalogDataType.h
 * @brief This file contains the declaration of the AnalogDataType class.
 */

#ifndef ANALOG_DATATYPE_H
#define ANALOG_DATATYPE_H

#include <limits>
#include <cstdint>

/**
 * @class AnalogDataType
 * @brief The AnalogDataType is a base class for the data types that will be used in analog computation models.
 *        It provides the functionality to set/get scale factor and to get the max limit of the type T.
 */
template <class T>
class AnalogDataType {
public:
    /**
     * @brief Constructor of the AnalogDataType class.
     */
    AnalogDataType()
        : scale_factor(1.0f), max_type_limit(std::numeric_limits<T>::max()) {}

    /**
     * @brief Sets the scale factor that will be used in the computations.
     * @param factor The scale factor to be set.
     */
    void set_scale_factor(float factor) {
        scale_factor = factor;
    }

    /**
     * @brief Gets the scale factor to be used in the computations.
     * @return The scale factor.
     */
    float get_scale_factor() const {
        return scale_factor;
    }

    /**
     * @brief Returns the maximum possible value for the type T.
     * @return The maximum possible value for the type T.
     */
    T get_max_type_limit() const {
        return max_type_limit;
    }

    /**
     * @brief Returns the current scale.
     * @return The current scale as a ratio of the scale factor and the max type limit.
     */
    float get_scale() const {
        return scale_factor / max_type_limit;
    }

private:
    float scale_factor; ///< The scale factor for quantization/dequantization.
    T max_type_limit; ///< The maximum possible value for the type T.
};

#endif // ANALOG_DATATYPE_H

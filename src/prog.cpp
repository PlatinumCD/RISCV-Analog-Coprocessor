#include <cstdint>

#include "mmio.h"
#include "analog/analog.h"

int main() {

    int rows, cols, length;
    float** mat;
    float* vec;

    readMatrixData(mat, rows, cols);
    readVectorData(vec, length);

    using mytype = int8_t;
    /*
     * Types in focus:
     *      int8_t
     *      int16_t
     *      int32_t
     *      int64_t
     */

    AnalogMatrix<mytype> analog_mat = AnalogMatrix<mytype>(mat, rows, cols);
    AnalogVector<mytype> analog_vec = AnalogVector<mytype>(vec, length);

    analog_mat.quantize();
    analog_vec.quantize();

    analog_mat.print();
    analog_vec.print();
}

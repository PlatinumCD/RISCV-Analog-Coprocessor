#include <cstdint>

#include "analog/analog.h"

int main() {

    int rows = 3;
    int cols = 3;

    float** mat = new float*[rows];
    float* vec = new float[cols];

    // Generate mat
    for (int i = 0; i < rows; i++) {
        mat[i] = new float[cols];
        for (int j = 0; j < cols; j++) {
            mat[i][j] = 4;//(float)rand()/(float)RAND_MAX*4 - 2; // Generates a random float between -2 and 2
        }
    }

    // Generate vec
    for (int i = 0; i < cols; i++) {
        vec[i] = 3;//(float)rand()/(float)RAND_MAX*4 - 2; // Generates a random float between -2 and 2
    }


    using mytype = int8_t;
    /*
     * Types in focus:
     *      int8_t
     *      int16_t
     *      int32_t
     *      int64_t
     */

    AnalogMatrix<mytype> analog_mat = AnalogMatrix<mytype>(mat, rows, cols);
    AnalogVector<mytype> analog_vec = AnalogVector<mytype>(vec, cols);

    /*
    analog_mat.quantize();
    analog_vec.quantize();

    analog_mat.print();
    analog_vec.print();
    */

    mvm_set_matrix(analog_mat, 0);
    mvm_load_vector(analog_vec, 0);
    mvm_compute(0);

    AnalogVector<mytype> analog_vec_out = AnalogVector<mytype>(cols);
    mvm_store_vector(analog_vec_out, 0);

    mvm_move_vector(0, 1);
}

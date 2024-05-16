#include <cstdint>
#include "analog/analog.h"

int main() {
    // Dimensions for matrix and vector
    const int rows = 3;
    const int cols = 4;

    // Allocate memory for matrix and vector
    float** mat = new float*[rows];
    float* vec = new float[cols];

    // Initialize matrix with default values and set specific elements
    for (int i = 0; i < rows; i++) {
        mat[i] = new float[cols];
        for (int j = 0; j < cols; j++) {
            mat[i][j] = 3.0f;
        }
    }
    mat[0][1] = 4.0f;
    mat[1][0] = 4.0f;
    mat[1][2] = 4.0f;
    mat[2][1] = 4.0f;

    // Initialize vector with default values and set specific element
    for (int i = 0; i < cols; i++) {
        vec[i] = 2.0f;
    }
    vec[1] = 1.0f;

    // Define input and output data types
    using input = int8_t;
    using output = int32_t;

    // Specify number of arrays for AnalogContext
    const int num_arrays = 1;
    AnalogContext ctx(num_arrays);

    // Create analog matrix and vectors
    AnalogMatrix<input> analog_mat(mat, rows, cols);
    AnalogVector<input> analog_vec(vec, cols);
    AnalogVector<output> analog_vec_out(cols);

    // Set matrix in the analog context and load the vector
    mvm_set_matrix(ctx, analog_mat, 0);
    mvm_load_vector(ctx, analog_vec, 0);

    // Print the matrix and vector
    analog_mat.print();
    analog_vec.print();

    // Perform matrix-vector multiplication
    mvm_compute(ctx, 0);

    // Store the resulting vector
    mvm_store_vector(ctx, analog_vec_out, 0);

    // Print the result
    analog_vec_out.print();

    // Clean up dynamically allocated memory
    for (int i = 0; i < rows; i++) {
        delete[] mat[i];
    }
    delete[] mat;
    delete[] vec;

    return 0;
}

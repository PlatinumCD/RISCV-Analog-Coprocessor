#include <cstdint>
#include "analog/analog.h"

int main() {
    // Dimensions for matrix and vector
    const int rows = 3;
    const int cols = 4;

    // Initialize matrix
    float** mat = new float*[rows];
    for (int i = 0; i < rows; i++) {
        mat[i] = new float[cols];
        for (int j = 0; j < cols; j++) {
            mat[i][j] = 3.0f;
        }
    }

    // Initialize vector
    float* vec = new float[cols];
    for (int i = 0; i < cols; i++) {
        vec[i] = 2.0f;
    }

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
    const int array_id = 0;
    mvm_set_matrix(ctx, analog_mat, array_id);
    mvm_load_vector(ctx, analog_vec, array_id);

    // Print the matrix and vector
    analog_mat.print();
    analog_vec.print();

    // Perform matrix-vector multiplication
    mvm_compute(ctx, array_id);

    // Store the resulting vector
    mvm_store_vector(ctx, analog_vec_out, array_id);

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

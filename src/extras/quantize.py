import numpy as np

def read_matrix_data(filename):
    with open(filename, 'r') as file:
        rows, cols = [int(x) for x in next(file).split()]
        matrix = [[float(x) for x in line.split()] for line in file]
    return np.array(matrix)

def read_vector_data(filename):
    with open(filename, 'r') as file:
        size = int(next(file))
        vector = [float(x) for x in next(file).split()]
    return np.array(vector)

def main():

    max_type_limit = 127 

    matrix = read_matrix_data('data/mat.data')
    vector = read_vector_data('data/vec.data')

    print('Expected out:')
    exp_out = matrix.dot(vector)
    print(exp_out)

    mat_scale_factor = np.max(np.abs(matrix))
    vec_scale_factor = np.max(np.abs(vector))

    quant_mat = np.round(matrix / mat_scale_factor * max_type_limit)
    quant_vec = np.round(vector / vec_scale_factor * max_type_limit)

    print()
    print('Quantized Matrix:')
    print(quant_mat)
    print()
    print('Quantized Vector:')
    print(quant_vec)
    print(vector)
    print(quant_vec * vec_scale_factor / max_type_limit)

    quant_out_vec = quant_mat.dot(quant_vec) / max_type_limit**2 * mat_scale_factor * vec_scale_factor

    print()
    print(quant_out_vec)


main()

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

    mat_a_max_type_limit = 127 
    mat_b_max_type_limit = 127 
    vec_max_type_limit = 127 

    matrix_a = read_matrix_data('data/mat.data')
    print('Matrix A in:')
    print(matrix_a)
    print()

    matrix_b = read_matrix_data('data/mat2.data')
    print('Matrix B in:')
    print(matrix_b)
    print()

    vector = read_vector_data('data/vec.data')
    print('Vector in:')
    print(vector)
    print()

    mat_a_scale_factor = np.max(np.abs(matrix_a))
    print('Matrix A scale factor: ', mat_a_scale_factor)

    mat_b_scale_factor = np.max(np.abs(matrix_b))
    print('Matrix B scale factor: ', mat_b_scale_factor)

    vec_scale_factor = np.max(np.abs(vector))
    print('Vector scale factor: ', vec_scale_factor)
    print()

    quant_mat_a = np.round(matrix_a / mat_a_scale_factor * mat_a_max_type_limit)
    print('Quantized Matrix A:')
    print(quant_mat_a)
    print()

    quant_mat_b = np.round(matrix_b / mat_b_scale_factor * mat_b_max_type_limit)
    print('Quantized Matrix B:')
    print(quant_mat_b)
    print()

    quant_vec = np.round(vector / vec_scale_factor * vec_max_type_limit)
    print('Quantized Vector:')
    print(quant_vec)
    print()

    quant_a_out_vec = quant_mat_a.dot(quant_vec)
    print('Quantized A output vec:')
    print(quant_a_out_vec)
    print()

    quant_b_out_vec = quant_mat_b.dot(quant_a_out_vec)
    print('Quantized B output vec:')
    print(quant_b_out_vec)
    print()

    mat_a_scale = mat_a_scale_factor / mat_a_max_type_limit
    print('Scale Matrix A: ', mat_a_scale)
    print()


    mat_b_scale = mat_b_scale_factor / mat_b_max_type_limit 
    print('Scale Matrix B: ', mat_b_scale)
    print()

    vec_scale = vec_scale_factor / vec_max_type_limit
    print('Scale Vector: ', vec_scale)
    print()

    dequant_out_vec = quant_b_out_vec * mat_a_scale * mat_b_scale * vec_scale
    print('Actual out:')
    print(dequant_out_vec)
    print()

    exp_out = matrix_b.dot(matrix_a.dot(vector))
    print('Expected out:')
    print(exp_out)


main()

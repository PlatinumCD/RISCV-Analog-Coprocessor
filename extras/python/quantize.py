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

    mat_max_type_limit = 127 
    vec_max_type_limit = 127 

    matrix = read_matrix_data('data/mat.data')
    print('Matrix in:')
    print(matrix)
    print()

    vector = read_vector_data('data/vec.data')
    print('Vector in:')
    print(vector)
    print()

    mat_scale_factor = np.max(np.abs(matrix))
    print('Matrix scale factor: ', mat_scale_factor)

    vec_scale_factor = np.max(np.abs(vector))
    print('Vector scale factor: ', vec_scale_factor)
    print()

    quant_mat = np.round(matrix / mat_scale_factor * mat_max_type_limit)
    print('Quantized Matrix:')
    print(quant_mat)
    print()

    quant_vec = np.round(vector / vec_scale_factor * vec_max_type_limit)
    print('Quantized Vector:')
    print(quant_vec)
    print()

    quant_out_vec = quant_mat.dot(quant_vec)
    print('Quantized output:')
    print(quant_out_vec)
    print()

    scale_factors = mat_scale_factor * vec_scale_factor
    max_type_limits = mat_max_type_limit * vec_max_type_limit
    scale = scale_factors / max_type_limits
    print('Scale: ', scale)
    print()

    dequant_out_vec = quant_out_vec * scale
    print('Actual out:')
    print(dequant_out_vec)
    print()

    exp_out = matrix.dot(vector)
    print('Expected out:')
    print(exp_out)




main()

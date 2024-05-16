#include <iostream>
#include <fstream>
#include <sstream>

void readMatrixData(float** &data, int &rows, int &cols) {
    std::ifstream inputFile("./data/mat.data");

    if (inputFile.is_open()) {
        inputFile >> rows;
        inputFile >> cols;

        data = new float*[rows];
        for(int i = 0; i < rows; ++i) {
            data[i] = new float[cols];
        }

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                inputFile >> data[i][j];
            }
        }

        inputFile.close();
    }
    else {
        std::cout << "Unable to open mat.data file" << std::endl;
    }
}

void readVectorData(float* &vec, int &size) {
    std::ifstream inputFile("./data/vec.data");

    if (inputFile.is_open()) {
        inputFile >> size;

        vec = new float[size];

        for (int i = 0; i < size; i++) {
            inputFile >> vec[i];
        }

        inputFile.close();
    }
    else {
        std::cout << "Unable to open vec.data file" << std::endl;
    }
}

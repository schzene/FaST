#include "mat-tools.h"

std::vector<double> matmul(
        std::vector<double>& mat1, 
        std::vector<double>& mat2, size_t dim1, size_t dim2, size_t dim3, bool trans) {
    std::vector<double> result(dim1 * dim3);
    size_t i, j, k;
    if (!trans) {
        for (i = 0; i < dim1; i++)
            for (j = 0; j < dim3; j++)
                for (k = 0; k < dim2; k++)
                    result[i * dim3 + j] += mat1[i * dim2 + k] * mat2[k * dim3 + j];
    } else {
        for (i = 0; i < dim1; i++)
            for (j = 0; j < dim3; j++)
                for (k = 0; k < dim2; k++)
                    result[i * dim3 + j] += mat1[i * dim2 + k] * mat2[j * dim3 + k];
    }
    return result;
}

void random_mat(std::vector<double>& mat) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-1, 1);

    size_t size = mat.size();
    for (size_t i = 0; i < size; i++) {
        mat[i] = dist(gen);
    }
}

std::vector<double> zero_sum(size_t row, size_t column) {
    std::vector<double> mat(row * column);
    random_mat(mat);
    size_t i, j;
    for (i = 0; i < row; i++) {
        double sum = 0.;
        for (j = 0; j < column - 1; j++) {
            sum += mat[i * column + j];
        }
        mat[(i + 1) * column - 1] = -sum;
    }
    return mat;
}

void norm(std::vector<double>& A) {
    auto max_num = A[0];
    auto size = A.size();
    for (size_t i = 1; i < size; i++)
        if (max_num < A[i]) max_num = A[i];
    for (size_t i = 0; i < size; i++) 
        A[i] -= max_num;
}

void print_mat(std::vector<double> A, size_t dim1, size_t dim2) {
    size_t i, j;
    for (i = 0; i < dim1; i++) {
        for (j = 0; j < dim2; j++) {
            std::cout << A[i * dim2 + j] << " ";
        }
        std::cout << "\n";
    }
}
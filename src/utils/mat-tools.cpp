#include "mat-tools.h"

matrix matmul(
    const matrix &mat1,
    const matrix &mat2, size_t dim1, size_t dim2, size_t dim3, bool trans) {
    matrix result(dim1 * dim3);
    if (!trans) {
        {
#pragma omp parallel for
            for (size_t i = 0; i < dim1; i++) {
                const size_t base_idx1 = i * dim2;
                const size_t base_idx2 = i * dim3;
                for (size_t k = 0; k < dim2; k++) {
                    const size_t base_idx3 = k * dim3;
                    const double tmp = mat1[base_idx1 + k];
                    for (size_t j = 0; j < dim3; j++) {
                        result[base_idx2 + j] += tmp * mat2[base_idx3 + j];
                    }
                }
            }
        }
    } else {
        {
#pragma omp parallel for
            for (size_t i = 0; i < dim1; i++) {
                const size_t base_idx1 = i * dim2;
                const size_t base_idx2 = i * dim3;
                for (size_t j = 0; j < dim3; j++) {
                    const size_t base_idx3 = j * dim2;
                    double sum = 0.;
                    for (size_t k = 0; k < dim2; k++) {
                        sum += mat1[base_idx1 + k] * mat2[base_idx3 + k];
                    }
                    result[base_idx2 + j] = sum;
                }
            }
        }
    }
    return result;
}

void random_mat(matrix &mat, double min, double max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(min, max);

    size_t size = mat.size();
    for (size_t i = 0; i < size; i++) {
        mat[i] = dist(gen);
    }
}

matrix zero_sum(size_t row, size_t column) {
    matrix mat(row * column);
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

void load_mat(matrix &mat, const char *path) {
    random_mat(mat);
}

void normalization(matrix &A, size_t row, size_t column) {
    size_t i, j;
    double max_value = 1ul << 20;
    for (i = 0; i < row * column; i++) {
        if (std::isnan(A[i]) || A[i] > max_value || -A[i] > max_value) {
            A[i] = 0;
        }
    }
    for (i = 0; i < row; i++) {
        auto max = A[i * column];
        for (j = 1; j < column; j++) {
            if (max < A[i * column + j]) {
                max = A[i * column + j];
            }
        }
        for (j = 0; j < column; j++) {
            A[i * column + j] -= max;
        }
    }
}

matrix mean(const matrix &input, size_t row, size_t column) {
    matrix result(row);
    size_t i, j;
    for (i = 0; i < row; i++) {
        for (j = 0; j < column; j++) {
            result[i] += input[i * column + j];
        }
        result[i] /= column;
    }
    return result;
}

matrix standard_deviation(const matrix &input, const matrix means, size_t row, size_t column) {
    matrix result(row);
    size_t i, j;
    for (i = 0; i < row; i++) {
        for (j = 0; j < column; j++) {
            result[i] += (input[i * column + j] - means[i]) * (input[i * column + j] - means[i]);
        }
        result[i] /= column;
        result[i] = sqrt(result[i]);
    }
    return result;
}

void print_mat(const matrix &A, size_t row, size_t column) {
    size_t i, j;
    bool flag1, flag2 = false;
    for (i = 0; i < row; i++) {
        flag1 = false;
        if (i < 5 || row - i < 5) {
            for (j = 0; j < column; j++) {
                if (j < 5 || column - j < 5) {
                    const double elem = A[i * column + j];
                    if (elem > 0) {
                    printf(" %-14lf", elem);
                    } else {
                        printf("%-15lf", elem);
                    }
                } else if (!flag1) {
                    printf("...   ");
                    flag1 = true;
                }
            }
            printf("\n");
        } else if (!flag2) {
            printf(" ...   \n");
            flag2 = true;
        }
    }
    cout << row << " x " << column << "\n";
}

void print_all_mat(const matrix &A, size_t row, size_t column) {
    size_t i, j;
    for (i = 0; i < row; i++) {
        for (j = 0; j < column; j++) {
            cout << A[i * column + j] << " ";
        }
        cout << "\n";
    }
}
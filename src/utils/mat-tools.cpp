#include "mat-tools.h"

matrix matmul(const matrix &mat1, const matrix &mat2,
              size_t dim1, size_t dim2, size_t dim3, bool trans) {
    matrix result(dim1 * dim3);
    if (!trans) {
        {
#ifdef _OPENMP
#pragma omp parallel for
#endif
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
#ifdef _OPENMP
#pragma omp parallel for
#endif
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

void random_mat(matrix &mat, double min, double max, bool binomial) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(min, max);

    size_t size = mat.size();
    for (size_t i = 0; i < size; i++) {
        mat[i] = dist(gen);
        if (binomial) {
            mat[i] = mat[i] > 0 ? max : min;
        }
    }
}

void random_bfv_mat(bfv_matrix &mat, int bw) {
    sci::PRG128 prg;
    size_t size = mat.size();
    uint64_t *rand_data = new uint64_t[size];
    prg.random_data(rand_data, size * sizeof(uint64_t));
    std::cout << mask << "\n";
    for (size_t i = 0; i < size; i++) {
        rand_data[i] &= mask;
        mat[i] = rand_data[i];
    }
    delete[] rand_data;
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
                    if (elem >= 0) {
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

LongCiphertext *RFCP_encodeA(const matrix &A, CKKSKey *party, CKKSEncoder *encoder,
                             size_t dim1, size_t dim2, size_t dim3) {
    matrix Ae(dim1 * dim2 * dim3);
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (size_t i = 0; i < dim2; i++) {
        for (size_t j = 0; j < dim1 * dim3; j++) {
            Ae[i * dim1 * dim3 + j] = A[j / dim3 * dim2 + i];
        }
    }

    LongCiphertext *lct = new LongCiphertext[dim2];
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (size_t i = 0; i < dim2; i++) {
        LongPlaintext lpt(matrix(Ae.begin() + dim1 * dim3 * i, Ae.begin() + dim1 * dim3 * (i + 1)), encoder);
        lct[i] = LongCiphertext(lpt, party);
    }
    return lct;
}

LongCiphertext RFCP_matmul(const LongCiphertext *A_secret,
                           const matrix &B,
                           size_t dim1, size_t dim2, size_t dim3,
                           CKKSEncoder *encoder, Evaluator *evaluator) {
    // we assume that A_secret has encoded
    matrix Be(dim1 * dim2 * dim3);
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (size_t i = 0; i < dim2; i++) {
        for (size_t j = 0; j < dim1 * dim3; j++) {
            Be[i * dim1 * dim3 + j] = B[i * dim3 + j % dim3];
        }
    }

    LongPlaintext lpt(matrix(Be.begin(), Be.begin() + dim1 * dim3), encoder);
    LongCiphertext result = A_secret[0].multiply_plain(lpt, evaluator);
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (size_t i = 1; i < dim2; i++) {
        LongPlaintext tmp_lpt(matrix(Be.begin() + dim1 * dim3 * i, Be.begin() + dim1 * dim3 * (i + 1)), encoder);
        LongCiphertext tmp_lct = A_secret[i].multiply_plain(tmp_lpt, evaluator);
#ifdef _OPENMP
#pragma omp critical
#endif
        {
            result.add_inplace(tmp_lct, evaluator);
        }
    }
    return result;
}
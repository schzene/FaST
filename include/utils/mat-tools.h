#ifndef FAST_MAT_TOOLS_H__
#define FAST_MAT_TOOLS_H__
#include <iostream>
#include <random>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "he-tools.h"
#include "io.h"

using std::cout;
using std::vector;

typedef vector<double> matrix;

matrix matmul(const matrix &mat1, const matrix &mat2,
              size_t dim1, size_t dim2, size_t dim3, bool trans = false);
void random_mat(matrix &mat, double min = -1., double max = 1., bool binomial = false);
matrix zero_sum(size_t row, size_t column);
void load_mat(matrix &mat, const char *path);
void normalization(matrix &A, size_t row, size_t column);
matrix mean(const matrix &input, size_t row, size_t column);
matrix standard_deviation(const matrix &input, const matrix means, size_t row, size_t column);
void print_mat(const matrix &A, size_t row, size_t column);
void print_all_mat(const matrix &A, size_t row, size_t column);
LongCiphertext *RFCP_encodeA(const matrix &A, CKKSKey *party, CKKSEncoder *encoder,
                             size_t dim1, size_t dim2, size_t dim3);
LongCiphertext RFCP_matmul(const LongCiphertext *A_secret, const matrix &B,
                               size_t dim1, size_t dim2, size_t dim3,
                               CKKSEncoder *encoder, Evaluator *evaluator);

inline void send_mat(IOPack *io_pack, matrix *mat, bool count_comm = true) {
    io_pack->send_data(mat->data(), mat->size() * sizeof(double), count_comm);
}

inline void recv_mat(IOPack *io_pack, matrix *mat, bool count_comm = true) {
    io_pack->recv_data(mat->data(), mat->size() * sizeof(double), count_comm);
}

#endif
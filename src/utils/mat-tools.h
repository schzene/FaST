#ifndef FAST_MAT_TOOLS_H__
#define FAST_MAT_TOOLS_H__
#include <iostream>
#include <random>
#include <vector>

#include "io.h"

std::vector<double> matmul(std::vector<double> &mat1, std::vector<double> &mat2, size_t dim1, size_t dim2, size_t dim3, bool trans = false);
void random_mat(std::vector<double> &mat, double min = -1., double max = 1.);
std::vector<double> zero_sum(size_t row, size_t column);
void load_mat(std::vector<double> &mat, const char *path);
void norm(std::vector<double> &A);
void print_mat(std::vector<double> A, size_t row, size_t column);
void print_all_mat(std::vector<double> A, size_t row, size_t column);

inline void send_mat(NetIO *io, std::vector<double> *mat) {
    io->send_data(mat->data(), mat->size() * sizeof(double));
}

inline void recv_mat(NetIO *io, std::vector<double> *mat) {
    io->recv_data(mat->data(), mat->size() * sizeof(double));
}

#endif
#ifndef FAST_MAT_TOOLS_H__
#define FAST_MAT_TOOLS_H__
#include <iostream>
#include <random>
#include <vector>

#include "io.h"

std::vector<double> matmul(const std::vector<double> &mat1, const std::vector<double> &mat2,
                           size_t dim1, size_t dim2, size_t dim3, bool trans = false);
void random_mat(std::vector<double> &mat, double min = -1., double max = 1.);
std::vector<double> zero_sum(size_t row, size_t column);
void load_mat(std::vector<double> &mat, const char *path);
void normalization(std::vector<double> &A, size_t row, size_t column);
std::vector<double> mean(const std::vector<double> &input, size_t row, size_t column);
std::vector<double> standard_deviation(const std::vector<double> &input, const std::vector<double> means, size_t row, size_t column);
void print_mat(const std::vector<double> &A, size_t row, size_t column);
void print_all_mat(const std::vector<double> &A, size_t row, size_t column);

inline void send_mat(IOPack *io_pack, std::vector<double> *mat) {
    io_pack->send_data(mat->data(), mat->size() * sizeof(double));
}

inline void recv_mat(IOPack *io_pack, std::vector<double> *mat) {
    io_pack->recv_data(mat->data(), mat->size() * sizeof(double));
}

#endif
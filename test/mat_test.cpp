#include <model.h>

matrix matmul1(
    const matrix &mat1,
    const matrix &mat2, size_t dim1, size_t dim2, size_t dim3, bool trans = false) {
    matrix result(dim1 * dim3);
    size_t i, j, k;
    if (!trans) {
        auto for_range = [&mat1, &mat2, &result, &dim1, &dim2, &dim3](size_t start, size_t end, mutex *mtx) {
            size_t i, j, k;
            for (i = start; i < end; i++) {
                for (k = 0; k < dim2; k++) {
                    for (j = 0; j < dim3; j++) {
                        result[i * dim3 + j] += mat1[i * dim2 + k] * mat2[k * dim3 + j];
                    }
                }
            }
        };
        for_acc(0, dim1, for_range);
    } else {
        for (i = 0; i < dim1; i++) {
            const size_t base_idx1 = i * dim2;
            const size_t base_idx2 = i * dim3;
            for (j = 0; j < dim3; j++) {
                const size_t base_idx3 = j * dim3;
                double sum = 0.;
                for (k = 0; k < dim2; k++) {
                    sum += mat1[base_idx1 + k] * mat2[base_idx3 + k];
                }
                result[base_idx2 + j] = sum;
            }
        }
    }
    return result;
}

matrix matmul2(
    const matrix &mat1,
    const matrix &mat2, size_t dim1, size_t dim2, size_t dim3, bool trans = false) {
    matrix result(dim1 * dim3);
    size_t i, j, k;
    if (!trans) {
        for (i = 0; i < dim1; i++) {
            const size_t base_idx1 = i * dim2;
            const size_t base_idx2 = i * dim3;
            for (k = 0; k < dim2; k++) {
                const size_t base_idx3 = k * dim3;
                const double tmp = mat1[base_idx1 + k];
                for (j = 0; j < dim3; j++) {
                    result[base_idx2 + j] += tmp * mat2[base_idx3 + j];
                }
            }
        }
    } else {
        for (i = 0; i < dim1; i++) {
            const size_t base_idx1 = i * dim2;
            const size_t base_idx2 = i * dim3;
            for (j = 0; j < dim3; j++) {
                const size_t base_idx3 = j * dim3;
                double sum = 0.;
                for (k = 0; k < dim2; k++) {
                    sum += mat1[base_idx1 + k] * mat2[base_idx3 + k];
                }
                result[base_idx2 + j] = sum;
            }
        }
    }
    return result;
}

int main() {
    matrix A(batch_size * d_module);
    matrix B(d_module * ffn_dim);
    INIT_TIMER

    START_TIMER
    auto result = matmul(A, B, batch_size, d_module, ffn_dim);
    STOP_TIMER("omp matmul")

    START_TIMER
    auto true_result = matmul1(A, B, batch_size, d_module, ffn_dim);
    STOP_TIMER("multithread matmul")

    START_TIMER
    auto true_result2 = matmul2(A, B, batch_size, d_module, ffn_dim);
    STOP_TIMER("matmul")
}
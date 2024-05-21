#include <model.h>

matrix matmul1(
    const matrix &mat1,
    const matrix &mat2, size_t dim1, size_t dim2, size_t dim3, bool trans = false) {
    matrix result(dim1 * dim3);
    if (!trans) {
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
    } else {
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
    return result;
}

matrix matmul2(
    const matrix &mat1,
    const matrix &mat2, size_t dim1, size_t dim2, size_t dim3, bool trans = false) {
    matrix result(dim1 * dim3);
    size_t i, j, k;
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
                for (size_t j = 0; j < dim3; j++) {
                    double sum = 0.;
                    for (size_t k = 0; k < dim2; k++) {
                        sum += mat1[i * dim2 + k] * mat2[j * dim2 + k];
                    }
                    result[i * dim3 + j] = sum;
                }
            }
        }
    }
    return result;
}

matrix mean1(const matrix &input, size_t row, size_t column) {
    matrix result(row);
    {
#pragma omp parallel for
        for (size_t i = 0; i < row; i++) {
            for (size_t j = 0; j < column; j++) {
                result[i] += input[i * column + j];
            }
            result[i] /= column;
        }
    }
    return result;
}

int main() {
    matrix A(batch_size * d_module);
    matrix B(batch_size * d_module);
    random_mat(A);
    for (size_t i = 0; i < batch_size * d_module; i++) {
        B[i] = 1e-9;
    }
    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    SEALContext *context = new SEALContext(parms);
    CKKSEncoder *encoder = new CKKSEncoder(*context);
    Evaluator *evaluator = new Evaluator(*context);
    CKKSKey *party = new CKKSKey(1, context);

    LongPlaintext A_plain(A, encoder), zero(B, encoder);
    LongCiphertext A_secret(A_plain, party);
    A_secret.multiply_plain_inplace(zero, evaluator);
    LongPlaintext A_p = A_secret.decrypt(party);
    auto A_ = A_p.decode(encoder);
    print_mat(A_, batch_size, d_module);
    // INIT_TIMER

    // START_TIMER
    // auto result = matmul(A, B, batch_size, d_module, batch_size);
    // STOP_TIMER("omp random_mat")

    // START_TIMER
    // auto true_result = matmul1(A, B, batch_size, d_module, batch_size);
    // STOP_TIMER("random_mat")

    // START_TIMER
    // auto result1 = matmul(A, B, batch_size, d_module, ffn_dim);
    // STOP_TIMER("multithread matmul")

    // START_TIMER
    // auto result2 = matmul1(A, B, batch_size, d_module, ffn_dim);
    // STOP_TIMER("omp matmul")

    // START_TIMER
    // auto true_result = matmul2(A, B, batch_size, d_module, ffn_dim);
    // STOP_TIMER("matmul")

    // std::cout << "error of multithread matmul\n";
    // for (size_t i = 0; i < batch_size * ffn_dim; i++) {
    //     result1[i] -= true_result[i];
    // }
    // print_mat(result1, batch_size, ffn_dim);

    // std::cout << "error of omp matmul\n";
    // for (size_t i = 0; i < batch_size * ffn_dim; i++) {
    //     result2[i] -= true_result[i];
    // }
    // print_mat(result2, batch_size, ffn_dim);
}
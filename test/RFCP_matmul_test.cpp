#include <module.h>

LongCiphertext RFCP_matmul(const LongCiphertext *A_secret,
                           const std::vector<double> &B,
                           size_t dim1, size_t dim2, size_t dim3,
                           CKKSEncoder *encoder, Evaluator *evaluator) {
    // we assume that A_secret has encoded
    size_t i, j, k;
    std::vector<double> Be(dim1 * dim2 * dim3);
    for (i = 0; i < dim2; i++) {
        for (j = 0; j < dim1 * dim3; j++) {
            Be[i * dim1 * dim3 + j] = B[i * dim3 + j % dim3];
        }
    }
    LongPlaintext *lpt = new LongPlaintext[dim2];
    for (i = 0; i < dim2; i++) {
        lpt[i] = LongPlaintext(std::vector<double>(Be.begin() + dim1 * dim3 * i, Be.begin() + dim1 * dim3 * (i + 1)), encoder);
    }
    LongCiphertext result = A_secret[0].multiply_plain(lpt[0], evaluator);
    for (i = 1; i < dim2; i++) {
        LongCiphertext tmp = A_secret[i].multiply_plain(lpt[i], evaluator);
        result.add_inplace(tmp, evaluator);
    }
    delete[] lpt;
    return result;
}

int main() {
    auto step = 2;
    size_t dim1 = d_module / step, dim2 = d_module / step, dim3 = d_module / step, i, j;
    std::vector<double> A(dim1 * dim2), B(dim2 * dim3);
    random_mat(A);
    random_mat(B);
    INIT_TIMER

    START_TIMER
    auto true_result = matmul(A, B, dim1, dim2, dim3);
    STOP_TIMER("matmul")

    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    SEALContext *context = new SEALContext(parms);
    CKKSEncoder *encoder = new CKKSEncoder(*context);
    Evaluator *evaluator = new Evaluator(*context);
    CKKSKey *party = new CKKSKey(1, context);

    std::vector<double> Ae(dim1 * dim2 * dim3);
    for (i = 0; i < dim2; i++) {
        for (j = 0; j < dim1 * dim3; j++) {
            Ae[i * dim1 * dim3 + j] = A[j / dim3 * dim2 + i];
        }
    }
    LongCiphertext *lct = new LongCiphertext[dim2];
    for (i = 0; i < dim2; i++) {
        LongPlaintext lpt(std::vector<double>(Ae.begin() + dim1 * dim3 * i, Ae.begin() + dim1 * dim3 * (i + 1)), encoder);
        lct[i] = LongCiphertext(lpt, party);
    }

    START_TIMER;
    auto result_secret = RFCP_matmul(lct, B, dim1, dim2, dim3, encoder, evaluator);
    STOP_TIMER("RFCP_matmul")
    auto result_plain = result_secret.decrypt(party);
    auto result = result_plain.decode(encoder);

    print_mat(true_result, dim1, dim3);
    print_mat(result, dim1, dim3);

    delete context;
    delete encoder;
    delete evaluator;
    delete party;
    delete[] lct;
    return 0;
}
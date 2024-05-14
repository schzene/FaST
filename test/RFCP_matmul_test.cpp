#include <model.h>

INIT_TIMER

LongCiphertext *RFCP_encodeA(const matrix &A, CKKSKey *party, CKKSEncoder *encoder,
                             size_t dim1, size_t dim2, size_t dim3) {
    matrix Ae(dim1 * dim2 * dim3);
#pragma omp parallal for
    for (size_t i = 0; i < dim2; i++) {
        for (size_t j = 0; j < dim1 * dim3; j++) {
            Ae[i * dim1 * dim3 + j] = A[j / dim3 * dim2 + i];
        }
    }

    LongCiphertext *lct = new LongCiphertext[dim2];
#pragma omp parallel for
    for (size_t i = 0; i < dim2; i++) {
        LongPlaintext lpt(matrix(Ae.begin() + dim1 * dim3 * i, Ae.begin() + dim1 * dim3 * (i + 1)), encoder);
        lct[i] = LongCiphertext(lpt, party);
    }
    return lct;
}

LongCiphertext RFCP_matmul_omp(const LongCiphertext *A_secret,
                               const matrix &B,
                               size_t dim1, size_t dim2, size_t dim3,
                               CKKSEncoder *encoder, Evaluator *evaluator) {
    // we assume that A_secret has encoded
    matrix Be(dim1 * dim2 * dim3);
#pragma omp parallel for
    for (size_t i = 0; i < dim2; i++) {
        for (size_t j = 0; j < dim1 * dim3; j++) {
            Be[i * dim1 * dim3 + j] = B[i * dim3 + j % dim3];
        }
    }

    LongPlaintext lpt(matrix(Be.begin(), Be.begin() + dim1 * dim3), encoder);
    LongCiphertext result = A_secret[0].multiply_plain(lpt, evaluator);
#pragma omp parallel for
    for (size_t i = 1; i < dim2; i++) {
        LongPlaintext tmp_lpt(matrix(Be.begin() + dim1 * dim3 * i, Be.begin() + dim1 * dim3 * (i + 1)), encoder);
        LongCiphertext tmp_lct = A_secret[i].multiply_plain(tmp_lpt, evaluator);
#pragma omp critical
        {
            result.add_inplace(tmp_lct, evaluator);
        }
    }
    return result;
}

LongCiphertext RFCP_matmul_multi_thread(const LongCiphertext *A_secret,
                                        const matrix &B,
                                        size_t dim1, size_t dim2, size_t dim3,
                                        CKKSEncoder *encoder, Evaluator *evaluator, const int thread_count = 12) {
    // we assume that A_secret has encoded
    matrix Be(dim1 * dim2 * dim3);
    {
#pragma omp parallal for
        for (size_t i = 0; i < dim2; i++) {
            for (size_t j = 0; j < dim1 * dim3; j++) {
                Be[i * dim1 * dim3 + j] = B[i * dim3 + j % dim3];
            }
        }
    }
    LongPlaintext lpt = LongPlaintext(matrix(Be.begin(), Be.begin() + dim1 * dim3), encoder);
    LongCiphertext result = A_secret[0].multiply_plain(lpt, evaluator);

    for_acc(1, dim2, [&Be, &dim1, &dim2, &dim3, A_secret, evaluator, encoder, &result](size_t start, size_t end, mutex *mtx) {
        LongPlaintext lpt_start = LongPlaintext(matrix(Be.begin() + dim1 * dim3 * start, Be.begin() + dim1 * dim3 * (start + 1)), encoder);
        LongCiphertext tmp1 = A_secret[start].multiply_plain(lpt_start, evaluator);

        for (size_t i = start + 1; i < end; i++) {
            LongPlaintext lpt = LongPlaintext(matrix(Be.begin() + dim1 * dim3 * i, Be.begin() + dim1 * dim3 * (i + 1)), encoder);
            LongCiphertext tmp = A_secret[i].multiply_plain(lpt, evaluator);
            tmp1.add_inplace(tmp, evaluator);
        }

        {
            std::lock_guard<mutex> lock(*mtx);
            result.add_inplace(tmp1, evaluator);
        }
    });
    return result;
}

int main() {
    std::cout << "////////////////////////////////////////////////////////////////////\n//                          _ooOoo_                               //\n//                         o8888888o                              //\n//                         88\" . \"88                              //\n//                         (| -_- |)                              //\n//                         O\\  =  /O                              //\n//                      ____/`---'\\____                           //\n//                    .'  \\\\|     |//  `.                         //\n//                   /  \\\\|||  :  |||//  \\                        //\n//                  /  _||||| -:- |||||-  \\                       //\n//                  |   | \\\\\\  -  /// |   |                       //\n//                  | \\_|  ''\\---/''  |   |                       //\n//                  \\  .-\\__  `-`  ___/-. /                       //\n//                ___`. .'  /--.--\\  `. . ___                     //\n//              .\"\" '<  `.___\\_<|>_/___.'  >'\"\".                  //\n//            | | :  `- \\`.;`\\ _ /`;.`/ - ` : | |                 //\n//            \\  \\ `-.   \\_ __\\ /__ _/   .-` /  /                 //\n//      ========`-.____`-.___\\_____/___.-`____.-'========         //\n//                           `=---='                              //\n//      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^        //\n//            佛祖保佑       永不宕机     永无BUG                 //\n////////////////////////////////////////////////////////////////////\n";
    auto step = 4;
    size_t dim1 = batch_size / step, dim2 = d_module / step, dim3 = ffn_dim / step, i, j;
    matrix A(dim1 * dim2), B(dim2 * dim3);
    random_mat(A);
    random_mat(B);

    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    SEALContext *context = new SEALContext(parms);
    CKKSEncoder *encoder = new CKKSEncoder(*context);
    Evaluator *evaluator = new Evaluator(*context);
    CKKSKey *party = new CKKSKey(1, context);

    START_TIMER
    auto true_result = matmul(A, B, dim1, dim2, dim3);
    STOP_TIMER("matmul");

    START_TIMER
    LongCiphertext *lct = RFCP_encodeA(A, party, encoder, dim1, dim2, dim3);
    STOP_TIMER("encode");

    START_TIMER
    auto result_secret = RFCP_matmul_multi_thread(lct, B, dim1, dim2, dim3, encoder, evaluator);
    STOP_TIMER("RFCP_matmul_multi_thread")
    auto result_plain = result_secret.decrypt(party);
    auto result = result_plain.decode(encoder);
    for (i = 0; i < dim1 * dim3; i++) {
        result[i] -= true_result[i];
    }
    std::cout << "error of multithread:\n";
    print_mat(result, dim1, dim3);

    START_TIMER
    auto result_secret2 = RFCP_matmul_omp(lct, B, dim1, dim2, dim3, encoder, evaluator);
    STOP_TIMER("RFCP_matmul_omp")
    auto result_plain2 = result_secret2.decrypt(party);
    auto result2 = result_plain2.decode(encoder);
    for (i = 0; i < dim1 * dim3; i++) {
        result2[i] -= true_result[i];
    }
    std::cout << "error of omp:\n";
    print_mat(result2, dim1, dim3);

    delete context;
    delete encoder;
    delete evaluator;
    delete party;
    delete[] lct;
    return 0;
}
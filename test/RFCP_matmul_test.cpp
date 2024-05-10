#include <model.h>
#include <mutex>
#include <thread>

using std::thread;

void for_acc(const size_t start, const size_t end,
             const std::function<void(size_t, size_t)> &for_range, const int threads_count = 12) {
    const size_t step = (end - start) / threads_count + 1;
    thread *threads = new thread[threads_count];

    for (int i = 0; i < threads_count; ++i) {
        size_t thread_start = start + i * step;
        size_t thread_end = std::min(thread_start + step, end);
        threads[i] = thread(for_range, thread_start, thread_end);
    }
    for (int i = 0; i < threads_count; ++i) {
        threads[i].join();
    }

    delete[] threads;
}

INIT_TIMER

LongCiphertext RFCP_matmul(const LongCiphertext *A_secret,
                           const matrix &B,
                           size_t dim1, size_t dim2, size_t dim3,
                           CKKSEncoder *encoder, Evaluator *evaluator) {
    // we assume that A_secret has encoded
    size_t i, j, k;

    matrix Be(dim1 * dim2 * dim3);
    for (i = 0; i < dim2; i++) {
        for (j = 0; j < dim1 * dim3; j++) {
            Be[i * dim1 * dim3 + j] = B[i * dim3 + j % dim3];
        }
    }

    LongPlaintext lpt = LongPlaintext(matrix(Be.begin(), Be.begin() + dim1 * dim3), encoder);
    LongCiphertext result = A_secret[0].multiply_plain(lpt, evaluator);
    for (i = 1; i < dim2; i++) {
        lpt = LongPlaintext(matrix(Be.begin() + dim1 * dim3 * i, Be.begin() + dim1 * dim3 * (i + 1)), encoder);
        LongCiphertext tmp = A_secret[i].multiply_plain(lpt, evaluator);
        result.add_inplace(tmp, evaluator);
    }
    return result;
}

LongCiphertext RFCP_matmul_multi_thread(const LongCiphertext *A_secret,
                                        const matrix &B,
                                        size_t dim1, size_t dim2, size_t dim3,
                                        CKKSEncoder *encoder, Evaluator *evaluator, const int thread_count = 12) {
    // we assume that A_secret has encoded
    size_t i, j, k;

    matrix Be(dim1 * dim2 * dim3);
    for (i = 0; i < dim2; i++) {
        for (j = 0; j < dim1 * dim3; j++) {
            Be[i * dim1 * dim3 + j] = B[i * dim3 + j % dim3];
        }
    }
    LongPlaintext lpt = LongPlaintext(matrix(Be.begin(), Be.begin() + dim1 * dim3), encoder);
    LongCiphertext result = A_secret[0].multiply_plain(lpt, evaluator);
    std::mutex mtx;

    auto for_range = [&Be, &dim1, &dim2, &dim3, &A_secret, evaluator, encoder, &result, &mtx](size_t start, size_t end) {
        LongPlaintext lpt_start = LongPlaintext(matrix(Be.begin() + dim1 * dim3 * start, Be.begin() + dim1 * dim3 * (start + 1)), encoder);
        LongCiphertext tmp1 = A_secret[start].multiply_plain(lpt_start, evaluator);

        for (size_t i = start + 1; i < end; i++) {
            LongPlaintext lpt = LongPlaintext(matrix(Be.begin() + dim1 * dim3 * i, Be.begin() + dim1 * dim3 * (i + 1)), encoder);
            LongCiphertext tmp = A_secret[i].multiply_plain(lpt, evaluator);
            tmp1.add_inplace(tmp, evaluator);
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            result.add_inplace(tmp1, evaluator);
        }
    };
    for_acc(1, dim2, for_range);
    return result;
}

int main() {
    std::cout <<"////////////////////////////////////////////////////////////////////\n//                          _ooOoo_                               //\n//                         o8888888o                              //\n//                         88\" . \"88                              //\n//                         (| -_- |)                              //\n//                         O\\  =  /O                              //\n//                      ____/`---'\\____                           //\n//                    .'  \\\\|     |//  `.                         //\n//                   /  \\\\|||  :  |||//  \\                        //\n//                  /  _||||| -:- |||||-  \\                       //\n//                  |   | \\\\\\  -  /// |   |                       //\n//                  | \\_|  ''\\---/''  |   |                       //\n//                  \\  .-\\__  `-`  ___/-. /                       //\n//                ___`. .'  /--.--\\  `. . ___                     //\n//              .\"\" '<  `.___\\_<|>_/___.'  >'\"\".                  //\n//            | | :  `- \\`.;`\\ _ /`;.`/ - ` : | |                 //\n//            \\  \\ `-.   \\_ __\\ /__ _/   .-` /  /                 //\n//      ========`-.____`-.___\\_____/___.-`____.-'========         //\n//                           `=---='                              //\n//      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^        //\n//            佛祖保佑       永不宕机     永无BUG                 //\n////////////////////////////////////////////////////////////////////\n";
    auto step = 1;
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

    matrix Ae(dim1 * dim2 * dim3);
    for (i = 0; i < dim2; i++) {
        for (j = 0; j < dim1 * dim3; j++) {
            Ae[i * dim1 * dim3 + j] = A[j / dim3 * dim2 + i];
        }
    }
    LongCiphertext *lct = new LongCiphertext[dim2];
    auto for_range = [&Ae, lct, party, encoder, &dim1, &dim3](size_t start, size_t end) {
        for (size_t i = start; i < end; i++) {
            LongPlaintext lpt(matrix(Ae.begin() + dim1 * dim3 * i, Ae.begin() + dim1 * dim3 * (i + 1)), encoder);
            lct[i] = LongCiphertext(lpt, party);
        }
    };
    for_acc(0, dim2, for_range);

    // START_TIMER
    // auto true_result_secret = RFCP_matmul(lct, B, dim1, dim2, dim3, encoder, evaluator);
    // STOP_TIMER("RFCP_matmul")
    // auto true_result_plain = true_result_secret.decrypt(party);
    // auto true_result = true_result_plain.decode(encoder);

    START_TIMER
    auto result_secret = RFCP_matmul_multi_thread(lct, B, dim1, dim2, dim3, encoder, evaluator);
    STOP_TIMER("RFCP_matmul_multi_thread")
    // auto result_plain = result_secret.decrypt(party);
    // auto result = result_plain.decode(encoder);
    // for (i = 0; i < dim1 * dim3; i++) {
    //     true_result[i] -= result[i];
    // }

    // std::cout << "error:\n";
    // print_mat(true_result, dim1, dim3);

    delete context;
    delete encoder;
    delete evaluator;
    delete party;
    delete[] lct;
    return 0;
}
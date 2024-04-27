#include <module.h>
#define TEST

class SecureAttention {
    CKKSKey *alice;
    CKKSKey *bob;
    CKKSEncoder *encoder;
    Evaluator *evaluator;
    size_t d_k;

public:
    SecureAttention(CKKSKey *alice_, CKKSKey *bob_,
                    CKKSEncoder *encoder_, Evaluator *evaluator_) : alice(alice_),
                                                                    bob(bob_),
                                                                    encoder(encoder_),
                                                                    evaluator(evaluator_) {}

    ~SecureAttention() {}

    void forward(const std::vector<double> &input) {
        size_t i, j;
        int d_k = d_module / n_heads;
        std::vector<double> ra_WQa(d_module * d_k),
            ra_WKa(d_module * d_k),
            ra_WVa(d_module * d_k),
            WQb(d_module * d_k),
            WKb(d_module * d_k),
            WVb(d_module * d_k);
        random_mat(ra_WQa);
        random_mat(ra_WKa);
        random_mat(ra_WVa);
        random_mat(WQb);
        random_mat(WKb);
        random_mat(WVb);
#ifdef TEST
        std::vector<double> WQ, WK, WV;
        for (i = 0; i < d_module * d_k; i++) {
            WQ.push_back(ra_WQa[i] + WQb[i]);
            WK.push_back(ra_WKa[i] + WKb[i]);
            WV.push_back(ra_WVa[i] + WVb[i]);
        }
#endif
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(-1, 1);
        double ra = dist(gen);

        std::vector<double> input_a(batch_size * d_module), input_b(batch_size * d_module), ra_xa(batch_size * d_module);
        random_mat(input_b, 0, 0.01);
        for (i = 0; i < batch_size * d_module; i++) {
            input_a[i] = input[i] - input_b[i];
            ra_xa[i] = ra * input_a[i];
        }
        for (i = 0; i < d_module * d_k; i++) {
            ra_WQa[i] = ra * ra_WQa[i];
            ra_WKa[i] = ra * ra_WKa[i];
            ra_WVa[i] = ra * ra_WVa[i];
        }

        auto ra_xa_WQa = matmul(input_a, ra_WQa, batch_size, d_module, d_k);
        auto ra_xa_WKa = matmul(input_a, ra_WKa, batch_size, d_module, d_k);
        auto ra_xa_WVa = matmul(input_a, ra_WVa, batch_size, d_module, d_k);

        LongCiphertext ra_secret_a(ra, alice, encoder);
        // send H1 = {ra_xa_WIa, ra_xa, ra_WIa, [ra]_a} to bob, where I = Q, K, V

        /*
            bob: revice H1, and possess: x_b, W_b
            1. compute: rxw_a + rx_a * w_b + rW_a * x_b + [r_a]_a * xw_b = [r_aI]_a , where I stands for  Q,K,V
            2. genereat random num r_b, compute [r_aQ/r_b]_a, [r_aK/r_b]_a, [(r_b)^2]_b
        */
        auto cal_raI_A = [](std::vector<double> input_b, std::vector<double> WIb, std::vector<double> ra_xa, std::vector<double> ra_WIa, std::vector<double> ra_xa_WIa,
                            LongCiphertext ra_secret_a,
                            CKKSKey *bob, CKKSEncoder *encoder, Evaluator *evaluator, size_t d_k) {
            auto xbWI_b = matmul(input_b, WIb, batch_size, d_module, d_k);
            LongPlaintext xbWI_b_plain(xbWI_b, encoder);
            LongCiphertext raI_secret_a = ra_secret_a.multiply_plain(xbWI_b_plain, evaluator);

            std::vector<double> temp_raI(batch_size * d_k);
            auto temp_raI1 = matmul(ra_xa, WIb, batch_size, d_module, d_k);
            auto temp_raI2 = matmul(input_b, ra_WIa, batch_size, d_module, d_k);
            for (size_t i = 0; i < batch_size * d_k; i++)
                temp_raI[i] = ra_xa_WIa[i] + temp_raI1[i] + temp_raI2[i];
            LongPlaintext temp_raI_plain(temp_raI, encoder);
            temp_raI_plain.mod_switch_to_inplace(raI_secret_a.parms_id(), evaluator);
            raI_secret_a.add_plain_inplace(temp_raI_plain, evaluator);
            return raI_secret_a;
        };
        // [r_aQ]_A
        LongCiphertext raQ_sec_a = cal_raI_A(input_b, WQb, ra_xa, ra_WQa, ra_xa_WQa, ra_secret_a, bob, encoder, evaluator, d_k);
        // [r_aK]_A
        LongCiphertext raK_sec_a = cal_raI_A(input_b, WKb, ra_xa, ra_WKa, ra_xa_WKa, ra_secret_a, bob, encoder, evaluator, d_k);
        // [r_aV]_A
        LongCiphertext raV_sec_a = cal_raI_A(input_b, WVb, ra_xa, ra_WVa, ra_xa_WVa, ra_secret_a, bob, encoder, evaluator, d_k);
#ifdef TEST1
        LongPlaintext raK_plain = raK_sec_a.decrypt(encoder, alice);
        auto raK = raK_plain.decode();
        for (i = 0; i < raK.size(); i++)
            raK[i] /= ra;
        print_mat(raK, batch_size, d_k);
#endif
        double rb1 = dist(gen);
        LongCiphertext rb1_square_secret_b(rb1 * rb1, bob, encoder);
        LongPlaintext div_rb1_plain(1. / rb1, encoder);
        div_rb1_plain.mod_switch_to_inplace(raQ_sec_a.parms_id(), evaluator);
        raQ_sec_a.multiply_plain_inplace(div_rb1_plain, evaluator);
        raK_sec_a.multiply_plain_inplace(div_rb1_plain, evaluator);
        // send H2 = {raQ_sec_a, raK_sec_a, rb1_square_secret_b} to alice

        /*
            alice receive H2, and get Q/rs1, K/rs1, [rb1]_s
        */
        LongPlaintext raQ_div_rb1_plain = raQ_sec_a.decrypt(alice);
        LongPlaintext raK_div_rb1_plain = raK_sec_a.decrypt(alice);
        std::vector<double> Q_div_rb1 = raQ_div_rb1_plain.decode(encoder);
        std::vector<double> K_div_rb1 = raK_div_rb1_plain.decode(encoder);
        std::vector<double> eZa(batch_size * batch_size);
        random_mat(eZa);
        std::vector<double> negZa(eZa);
        auto sqrt_d_k = sqrt(d_k);
        for (size_t i = 0; i < batch_size * d_k; i++) {
            Q_div_rb1[i] /= ra;
            Q_div_rb1[i] /= sqrt_d_k;
            K_div_rb1[i] /= ra;
        }
        auto temp_z = matmul(Q_div_rb1, K_div_rb1, batch_size, d_k, batch_size, true);
        for (size_t i = 0; i < batch_size * batch_size; i++) {
            negZa[i] = -negZa[i];
            eZa[i] = exp(eZa[i]);
        }
        normalization(temp_z, batch_size, batch_size);
        LongPlaintext z_plain(temp_z, encoder);
        auto Zb_secret_b = rb1_square_secret_b.multiply_plain(z_plain, evaluator);
#ifdef TEST1
        auto Zb_plain = Zb_secret_b.decrypt(encoder, bob);
        auto Zb1 = Zb_plain.decode();
        print_mat(Zb1, batch_size, batch_size);
#endif
        LongPlaintext negZc_plain(negZa, encoder);
        negZc_plain.mod_switch_to_inplace(Zb_secret_b.parms_id(), evaluator);
        Zb_secret_b.add_plain_inplace(negZc_plain, evaluator);

        LongPlaintext eZa_plain(eZa, encoder);
        LongCiphertext eZa_secret_a(eZa_plain, alice);
        // send H3 = {Zb_secret_b, eZa_secret_a} to bob

        /*
            bob receive H3, and get Zb, [exp(Zc)]_a
        */
        LongPlaintext eZb_plain = Zb_secret_b.decrypt(bob);
        std::vector<double> eZb = eZb_plain.decode(encoder);
        double rb2 = dist(gen);
#ifdef TEST1
        rb2 = 1;
#endif
        std::vector<double> Db(batch_size);
        random_mat(Db);
        std::vector<double> O = zero_sum(batch_size, batch_size);
        for (size_t i = 0; i < batch_size * batch_size; i++) {
            eZb[i] = exp(eZb[i]) * rb2;
#ifdef TEST1
            Db[i / batch_size] = 1;
#endif
        }
#ifdef TEST1
        std::vector<double> Z(batch_size * batch_size);
        for (i = 0; i < batch_size * batch_size; i++)
            Z[i] = eZb[i] * eZa[i];
        print_mat(Z, batch_size, batch_size);
#endif
        LongPlaintext r2s_expZb_plain(eZb, encoder);
        eZa_secret_a.multiply_plain_inplace(r2s_expZb_plain, evaluator);
        LongPlaintext O_plain(O, encoder);
        O_plain.mod_switch_to_inplace(eZa_secret_a.parms_id(), evaluator);
        eZa_secret_a.add_plain_inplace(O_plain, evaluator);

        for (size_t i = 0; i < batch_size * batch_size; i++)
            eZb[i] = eZb[i] * Db[i / batch_size] / rb2;
        std::vector<double> Rb(batch_size * d_k);
        random_mat(Rb);
        for (i = 1; i < batch_size; i++)
            for (j = 0; j < d_k; j++)
                Rb[i * d_k + j] = Rb[j];
#ifdef TEST1
        for (i = 0; i < batch_size; i++)
            for (j = 0; j < d_module; j++)
                Rb[i * d_module + j] = 1;
#endif
        LongPlaintext Rb_plain(Rb, encoder);
        Rb_plain.mod_switch_to_inplace(raV_sec_a.parms_id(), evaluator);
        raV_sec_a.multiply_plain_inplace(Rb_plain, evaluator);

        std::vector<double> Zb(batch_size * d_k);
        for (i = 0; i < batch_size; i++)
            for (j = 0; j < d_k; j++)
                Zb[i * d_k + j] = rb2 / (Db[i] * Rb[j]);
        // send H4 = {eZa_secret_a, eZb, raV_sec_a} to alice

        /*
            alice receive H4, and get rs2 * exp(Z) + O, Db * exp(Zs), Rb * V,
        */
        LongPlaintext rs2_expZ_plain = eZa_secret_a.decrypt(alice);
        auto rs2_expZ = rs2_expZ_plain.decode(encoder);

        LongPlaintext Rb_V_plain = raV_sec_a.decrypt(alice);
        auto Rb_V = Rb_V_plain.decode(encoder);
        for (size_t i = 0; i < batch_size * d_k; i++)
            Rb_V[i] /= ra;

        std::vector<double> exp_sum(batch_size);
        for (size_t i = 0; i < batch_size; i++) {
            for (j = 0; j < batch_size; j++) {
                exp_sum[i] += rs2_expZ[i * batch_size + j];
            }
        }
#ifdef TEST1
        print_mat(exp_sum, batch_size, 1);
#endif
        for (i = 0; i < batch_size; i++) {
            for (j = 0; j < batch_size; j++) {
                eZb[i * batch_size + j] *= eZa[i * batch_size + j];
                eZb[i * batch_size + j] /= exp_sum[i];
            }
        }
#ifdef TEST1
        print_mat(eZb, batch_size, batch_size);
#endif
        auto Za = matmul(eZb, Rb_V, batch_size, batch_size, d_k);

        std::cout << "Secure Attention done.\n";
#ifdef TEST
        auto Q = matmul(input, WQ, batch_size, d_module, d_k);
        auto K = matmul(input, WK, batch_size, d_module, d_k);
        auto V = matmul(input, WV, batch_size, d_module, d_k);
        // print_mat(K, batch_size, d_k);
        auto QK = matmul(Q, K, batch_size, d_k, batch_size, true);
        normalization(QK, batch_size, batch_size);
        for (i = 0; i < QK.size(); i++) {
            QK[i] /= sqrt_d_k;
            QK[i] = exp(QK[i]);
        }
        // print_mat(QK, batch_size, batch_size);
        std::vector<double> exp_sum1(batch_size);
        for (i = 0; i < batch_size; i++)
            for (j = 0; j < batch_size; j++)
                exp_sum1[i] += QK[i * batch_size + j];
        for (size_t i = 0; i < batch_size; i++) {
            for (j = 0; j < batch_size; j++) {
                QK[i * batch_size + j] /= exp_sum1[i];
            }
        }
        // print_mat(exp_sum1, batch_size, 1);
        // print_mat(QK, batch_size, batch_size);
        auto result = matmul(QK, V, batch_size, batch_size, d_k);
        std::vector<double> Z(batch_size * d_k);
        for (i = 0; i < batch_size * d_k; i++) {
            Z[i] = Za[i] * Zb[i] - result[i];
            if (Z[i] < 0) {
                Z[i] = -Z[i];
            }
        }
        std::cout << "error:"
                  << "\n";
        print_mat(Z, batch_size, d_k);
#endif
    }
};

int main() {
    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    SEALContext *context = new SEALContext(parms);
    CKKSEncoder *encoder = new CKKSEncoder(*context);
    Evaluator *evaluator = new Evaluator(*context);

    size_t d_k = d_module / n_heads;
    CKKSKey *alice = new CKKSKey(1, context);
    CKKSKey *bob = new CKKSKey(2, context);
    std::vector<double> input(batch_size * d_module);
    random_mat(input, 0, 0.01);
    SecureAttention *sattn = new SecureAttention(alice, bob, encoder, evaluator);
    sattn->forward(input);

    /*std::vector<double> input1(8193);
    std::vector<double> input2(8193);
    random_mat(input1);
    random_mat(input2);
    LongPlaintext input1_plain(input1, 1ul<<40, slot_count, encoder);
    LongPlaintext input2_plain(input2, 1ul<<40, slot_count, encoder);
    LongCiphertext input1_secret(input1_plain, alice);*/

    // test add_plain_inplace......pass
    /*input1_secret.add_plain_inplace(input2_plain, evaluator);
    auto result_plain = input1_secret.decrypt(encoder, alice);
    auto result = result_plain.decode();
    for (size_t i = 0; i < 8193; i++) {
        result[i] -= (input1[i] + input2[i]);
        if (result[i] < 0) result[i] = -result[i];
    }
    */
    // test multiply_plain.....pass
    /*auto result_secret = input1_secret.multiply_plain(input2_plain, evaluator);
    auto result_plain = result_secret.decrypt(encoder, alice);
    auto result = result_plain.decode();
    for (size_t i = 0; i < 8193; i++) {
        result[i] -= (input1[i] * input2[i]);
        if (result[i] < 0) result[i] = -result[i];
    }*/
    /*
    input1_secret.multiply_plain_inplace(input2_plain, evaluator);
    auto result_plain = input1_secret.decrypt(encoder, alice);
    auto result = result_plain.decode();
    for (size_t i = 0; i < 8193; i++) {
        result[i] -= (input1[i] * input2[i]);
        if (result[i] < 0) result[i] = -result[i];
    }
    auto max_error = max(result);
    std::cout << max_error << "\n";
    */

    /* std::vector<double> very_small_numbers = {2.43e-8};
    Plaintext pt;
    encoder->encode(very_small_numbers, 1ul << 40, pt); // 1e-8
    // Ciphertext ct; alice->encryptor->encrypt(pt, ct); // 1e-8
    // Plaintext res_plain; alice->decryptor->decrypt(ct, res_plain);
    std::vector<double> res;
    encoder->decode(pt, res);
    // encoder->decode(res_plain, res);
    std::cout << res[0] << "\n"; */

    delete context;
    delete encoder;
    delete evaluator;
    delete sattn;
    delete alice;
    delete bob;
}
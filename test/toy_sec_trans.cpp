#include <utils.h>
#define TEST

class SecureTransformer {
    CKKSKey *alice;
    CKKSKey *bob;
    CKKSEncoder *encoder;
    Evaluator *evaluator;
    std::vector<double> input;
    size_t d_module, d_k, n_heads;

public:
    double scale = 1ul << 40;
    SecureTransformer(CKKSKey *alice_, CKKSKey *bob_, std::vector<double> input_,
                      size_t d_module_, size_t d_k_, size_t n_heads_) : alice(alice_),
                                                                        bob(bob_),
                                                                        input(input_),
                                                                        d_module(d_module_),
                                                                        d_k(d_k_),
                                                                        n_heads(n_heads_) {
        encoder = new CKKSEncoder(*(alice->context));
        evaluator = new Evaluator(*(alice->context));
    }

    ~SecureTransformer() {
        delete encoder;
        delete evaluator;
    }

    void attn() {
        size_t i, j;
        size_t inp_seq = this->input.size() / d_module;
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

        std::vector<double> input_a(inp_seq * d_module), input_b(inp_seq * d_module), ra_xa(inp_seq * d_module);
        random_mat(input_b, 0, 0.01);
        for (i = 0; i < inp_seq * d_module; i++) {
            input_a[i] = input[i] - input_b[i];
            ra_xa[i] = ra * input_a[i];
        }
        for (i = 0; i < d_module * d_k; i++) {
            ra_WQa[i] = ra * ra_WQa[i];
            ra_WKa[i] = ra * ra_WKa[i];
            ra_WVa[i] = ra * ra_WVa[i];
        }

        auto ra_xa_WQa = matmul(input_a, ra_WQa, inp_seq, d_module, d_k);
        auto ra_xa_WKa = matmul(input_a, ra_WKa, inp_seq, d_module, d_k);
        auto ra_xa_WVa = matmul(input_a, ra_WVa, inp_seq, d_module, d_k);

        Plaintext ra_plain;
        encoder->encode(ra, scale, ra_plain);
        Ciphertext ra_secret_a_;
        alice->encryptor->encrypt(ra_plain, ra_secret_a_);
        LongCiphertext ra_secret_a(ra_secret_a_);
        // send H1 = {ra_xa_WIa, ra_xa, ra_WIa, [ra]_a} to bob, where I = Q, K, V

        /*
            Bob: revice H1, and possess: x_b, W_b
            1. compute: rxw_a + rx_a * w_b + rW_a * x_b + [r_a]_a * xw_b = [r_aI]_a , where I stands for  Q,K,V
            2. genereat random num r_b, compute [r_aQ/r_b]_a, [r_aK/r_b]_a, [(r_b)^2]_b
        */
        auto cal_raI_A = [](std::vector<double> input_b, std::vector<double> WIb, std::vector<double> ra_xa, std::vector<double> ra_WIa, std::vector<double> ra_xa_WIa,
                            LongCiphertext ra_secret_a,
                            CKKSKey *bob, CKKSEncoder *encoder, Evaluator *evaluator,
                            double scale, size_t inp_seq, size_t d_module, size_t d_k, size_t n_heads) {
            auto xbWI_b = matmul(input_b, WIb, inp_seq, d_module, d_k);
            LongPlaintext xbWI_b_plain(xbWI_b, scale, bob->slot_count, encoder);
            LongCiphertext raI_secret_a = ra_secret_a.multiply_plain(xbWI_b_plain, evaluator);
            raI_secret_a.rescale_to_next_inplace(evaluator);
            raI_secret_a.scale(scale);

            std::vector<double> temp_raI(inp_seq * d_k);
            auto temp_raI1 = matmul(ra_xa, WIb, inp_seq, d_module, d_k);
            auto temp_raI2 = matmul(input_b, ra_WIa, inp_seq, d_module, d_k);
            for (size_t i = 0; i < inp_seq * d_k; i++)
                temp_raI[i] = ra_xa_WIa[i] + temp_raI1[i] + temp_raI2[i];
            LongPlaintext temp_raI_plain(temp_raI, scale, bob->slot_count, encoder);
            temp_raI_plain.mod_switch_to_inplace(raI_secret_a.parms_id(), evaluator);
            raI_secret_a.add_plain_inplace(temp_raI_plain, evaluator);
            return raI_secret_a;
        };
        // [r_aQ]_A
        LongCiphertext raQ_sec_a = cal_raI_A(input_b, WQb, ra_xa, ra_WQa, ra_xa_WQa, ra_secret_a, bob, encoder, evaluator, scale, inp_seq, d_module, d_k, n_heads);
        // [r_aK]_A
        LongCiphertext raK_sec_a = cal_raI_A(input_b, WKb, ra_xa, ra_WKa, ra_xa_WKa, ra_secret_a, bob, encoder, evaluator, scale, inp_seq, d_module, d_k, n_heads);
        // [r_aV]_A
        LongCiphertext raV_sec_a = cal_raI_A(input_b, WVb, ra_xa, ra_WVa, ra_xa_WVa, ra_secret_a, bob, encoder, evaluator, scale, inp_seq, d_module, d_k, n_heads);
#ifdef TEST1
        LongPlaintext raK_plain = raK_sec_a.decrypt(encoder, alice);
        auto raK = raK_plain.decode();
        for (i = 0; i < raK.size(); i++)
            raK[i] /= ra;
        print_mat(raK, inp_seq, d_k);
#endif
        double rb1 = dist(gen), div_rb1 = 1 / rb1, rb1_square = rb1 * rb1;
        Plaintext div_rb1_plain_;
        encoder->encode(div_rb1, scale, div_rb1_plain_);
        Plaintext rb1_square_plain;
        encoder->encode(rb1_square, scale, rb1_square_plain);
        Ciphertext rb1_square_secret_b_;
        bob->encryptor->encrypt(rb1_square_plain, rb1_square_secret_b_);
        LongCiphertext rb1_square_secret_b(rb1_square_secret_b_);
        LongPlaintext div_rb1_plain(div_rb1_plain_, bob->slot_count);
        div_rb1_plain.mod_switch_to_inplace(raQ_sec_a.parms_id(), evaluator);
        raQ_sec_a.multiply_plain_inplace(div_rb1_plain, evaluator);
        raK_sec_a.multiply_plain_inplace(div_rb1_plain, evaluator);
        // send H2 = {raQ_sec_a, raK_sec_a, rb1_square_secret_b} to Alice

        /*
            Alice receive H2, and get Q/rs1, K/rs1, [rb1]_s
        */
        LongPlaintext raQ_div_rb1_plain = raQ_sec_a.decrypt(alice);
        LongPlaintext raK_div_rb1_plain = raK_sec_a.decrypt(alice);
        std::vector<double> Q_div_rb1 = raQ_div_rb1_plain.decode(encoder);
        std::vector<double> K_div_rb1 = raK_div_rb1_plain.decode(encoder);
        std::vector<double> eZa(inp_seq * inp_seq);
        random_mat(eZa);
        std::vector<double> negZa(eZa);
        auto sqrt_d_k = sqrt(d_k);
        for (size_t i = 0; i < inp_seq * d_k; i++) {
            Q_div_rb1[i] /= ra;
            Q_div_rb1[i] /= sqrt_d_k;
            K_div_rb1[i] /= ra;
        }
        auto temp_z = matmul(Q_div_rb1, K_div_rb1, inp_seq, d_k, inp_seq, true);
        for (size_t i = 0; i < inp_seq * inp_seq; i++) {
            negZa[i] = -negZa[i];
            eZa[i] = exp(eZa[i]);
        }
        norm(temp_z, inp_seq, inp_seq);
        LongPlaintext z_plain(temp_z, scale, alice->slot_count, encoder);
        auto Zb_secret_b = rb1_square_secret_b.multiply_plain(z_plain, evaluator);
#ifdef TEST1
        auto Zb_plain = Zb_secret_b.decrypt(encoder, bob);
        auto Zb1 = Zb_plain.decode();
        print_mat(Zb1, inp_seq, inp_seq);
#endif
        Zb_secret_b.rescale_to_next_inplace(evaluator);
        Zb_secret_b.scale(scale);
        LongPlaintext negZc_plain(negZa, scale, alice->slot_count, encoder);
        negZc_plain.mod_switch_to_inplace(Zb_secret_b.parms_id(), evaluator);
        Zb_secret_b.add_plain_inplace(negZc_plain, evaluator);

        LongPlaintext eZa_plain(eZa, scale, alice->slot_count, encoder);
        LongCiphertext eZa_secret_a(eZa_plain, alice);
        // send H3 = {Zb_secret_b, eZa_secret_a} to Bob

        /*
            Bob receive H3, and get Zb, [exp(Zc)]_a
        */
        LongPlaintext eZb_plain = Zb_secret_b.decrypt(bob);
        std::vector<double> eZb = eZb_plain.decode(encoder);
        double rb2 = dist(gen);
#ifdef TEST1
        rb2 = 1;
#endif
        std::vector<double> Db(inp_seq);
        random_mat(Db);
        std::vector<double> O = zero_sum(inp_seq, inp_seq);
        for (size_t i = 0; i < inp_seq * inp_seq; i++) {
            eZb[i] = exp(eZb[i]) * rb2;
#ifdef TEST1
            Db[i / inp_seq] = 1;
#endif
        }
#ifdef TEST1
        std::vector<double> Z(inp_seq * inp_seq);
        for (i = 0; i < inp_seq * inp_seq; i++)
            Z[i] = eZb[i] * eZa[i];
        print_mat(Z, inp_seq, inp_seq);
#endif
        LongPlaintext r2s_expZb_plain(eZb, scale, bob->slot_count, encoder);
        eZa_secret_a.multiply_plain_inplace(r2s_expZb_plain, evaluator);
        eZa_secret_a.rescale_to_next_inplace(evaluator);
        eZa_secret_a.scale(scale);
        LongPlaintext O_plain(O, scale, bob->slot_count, encoder);
        O_plain.mod_switch_to_inplace(eZa_secret_a.parms_id(), evaluator);
        eZa_secret_a.add_plain_inplace(O_plain, evaluator);

        for (size_t i = 0; i < inp_seq * inp_seq; i++)
            eZb[i] = eZb[i] * Db[i / inp_seq] / rb2;
        std::vector<double> Rb(inp_seq * d_k);
        random_mat(Rb);
        for (i = 1; i < inp_seq; i++)
            for (j = 0; j < d_k; j++)
                Rb[i * d_k + j] = Rb[j];
#ifdef TEST1
        for (i = 0; i < inp_seq; i++)
            for (j = 0; j < d_module; j++)
                Rb[i * d_module + j] = 1;
#endif
        LongPlaintext Rb_plain(Rb, scale, bob->slot_count, encoder);
        Rb_plain.mod_switch_to_inplace(raV_sec_a.parms_id(), evaluator);
        raV_sec_a.multiply_plain_inplace(Rb_plain, evaluator);

        std::vector<double> Zb(inp_seq * d_k);
        for (i = 0; i < inp_seq; i++)
            for (j = 0; j < d_k; j++)
                Zb[i * d_k + j] = rb2 / (Db[i] * Rb[j]);
        // send H4 = {eZa_secret_a, eZb, raV_sec_a} to alice

        /*
            Alice receive H4, and get rs2 * exp(Z) + O, Db * exp(Zs), Rb * V,
        */
        LongPlaintext rs2_expZ_plain = eZa_secret_a.decrypt(alice);
        auto rs2_expZ = rs2_expZ_plain.decode(encoder);

        LongPlaintext Rb_V_plain = raV_sec_a.decrypt(alice);
        auto Rb_V = Rb_V_plain.decode(encoder);
        for (size_t i = 0; i < inp_seq * d_k; i++)
            Rb_V[i] /= ra;

        std::vector<double> exp_sum(inp_seq);
        for (size_t i = 0; i < inp_seq; i++) {
            for (j = 0; j < inp_seq; j++) {
                exp_sum[i] += rs2_expZ[i * inp_seq + j];
            }
        }
#ifdef TEST1
        print_mat(exp_sum, inp_seq, 1);
#endif
        for (i = 0; i < inp_seq; i++) {
            for (j = 0; j < inp_seq; j++) {
                eZb[i * inp_seq + j] *= eZa[i * inp_seq + j];
                eZb[i * inp_seq + j] /= exp_sum[i];
            }
        }
#ifdef TEST1
        print_mat(eZb, inp_seq, inp_seq);
#endif
        auto Za = matmul(eZb, Rb_V, inp_seq, inp_seq, d_k);

        std::cout << "Secure Attention done.\n";
#ifdef TEST
        auto Q = matmul(input, WQ, inp_seq, d_module, d_k);
        auto K = matmul(input, WK, inp_seq, d_module, d_k);
        auto V = matmul(input, WV, inp_seq, d_module, d_k);
        // print_mat(K, inp_seq, d_k);
        auto QK = matmul(Q, K, inp_seq, d_k, inp_seq, true);
        norm(QK, inp_seq, inp_seq);
        for (i = 0; i < QK.size(); i++) {
            QK[i] /= sqrt_d_k;
            QK[i] = exp(QK[i]);
        }
        // print_mat(QK, inp_seq, inp_seq);
        std::vector<double> exp_sum1(inp_seq);
        for (i = 0; i < inp_seq; i++)
            for (j = 0; j < inp_seq; j++)
                exp_sum1[i] += QK[i * inp_seq + j];
        for (size_t i = 0; i < inp_seq; i++) {
            for (j = 0; j < inp_seq; j++) {
                QK[i * inp_seq + j] /= exp_sum1[i];
            }
        }
        // print_mat(exp_sum1, inp_seq, 1);
        // print_mat(QK, inp_seq, inp_seq);
        auto result = matmul(QK, V, inp_seq, inp_seq, d_k);
        std::vector<double> Z(inp_seq * d_k);
        for (i = 0; i < inp_seq * d_k; i++) {
            Z[i] = Za[i] * Zb[i] - result[i];
            if (Z[i] < 0) {
                Z[i] = -Z[i];
            }
        }
        std::cout << "errer:" <<"\n";
        print_mat(Z, inp_seq, d_k);
#endif
    }
};

int main() {
    size_t poly_modulus_degree = 8192;
    size_t slot_count = poly_modulus_degree / 2;
    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    SEALContext *context = new SEALContext(parms);
    CKKSEncoder *encoder = new CKKSEncoder(*context);
    Evaluator *evaluator = new Evaluator(*context);

    size_t inp_seq = 96, d_module = 768, d_k = 64, n_heads = 5;
    CKKSKey *alice = new CKKSKey(1, context, slot_count);
    CKKSKey *bob = new CKKSKey(2, context, slot_count);
    std::vector<double> input(inp_seq * d_module);
    random_mat(input, 0, 0.01);
    SecureTransformer* st = new SecureTransformer(alice, bob, input, d_module, d_k, n_heads);
    st->attn();

    auto max = [](std::vector<double> z) {
        auto size = z.size() - 1;
        if (size < 0)
            return 0.;
        auto max_num = z[0];
        for (; size >= 1; size--)
            if (max_num < z[size])
                max_num = z[size];
        return max_num;
    };
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

    // delete st;
    delete context;
    delete alice;
    delete bob;
}
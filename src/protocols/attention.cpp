#include "attention.h"
std::vector<double> Attention::forward(const std::vector<double> &input) const {
    size_t i, j;
    size_t d_k = d_module / n_heads;
    std::vector<double> WQ(d_module * d_k), WK(d_module * d_k), WV(d_module * d_k);
    load_mat(WQ, "WQa-${party}");
    load_mat(WK, "WKa-${party}");
    load_mat(WV, "WVa-${party}");
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-1, 1);
    if (party->party == ALICE) {
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif
        // alice: possess: x_a, W_a
        double ra = dist(gen);
        std::vector<double> ra_xa(batch_size * d_module);
        for (i = 0; i < batch_size * d_k; i++) {
            ra_xa[i] = ra * input[i];
        }
        for (i = 0; i < d_module * d_k; i++) {
            WQ[i] = ra * WQ[i];
            WK[i] = ra * WK[i];
            WV[i] = ra * WV[i];
        }
        auto ra_xa_WQa = matmul(input, WQ, batch_size, d_module, d_k);
        auto ra_xa_WKa = matmul(input, WK, batch_size, d_module, d_k);
        auto ra_xa_WVa = matmul(input, WV, batch_size, d_module, d_k);
        LongCiphertext ra_secret_a(ra, party, encoder);
        // send H1 = {ra_xa_WIa, ra_xa, ra_WIa, [ra]_a} to bob, where I = Q, K, V
        send_mat(io_pack, &ra_xa_WQa);
        send_mat(io_pack, &ra_xa_WKa);
        send_mat(io_pack, &ra_xa_WVa);
        send_mat(io_pack, &ra_xa);
        send_mat(io_pack, &WQ);
        send_mat(io_pack, &WK);
        send_mat(io_pack, &WV);
        LongCiphertext::send(io_pack, &ra_secret_a);

        /*
            alice receive H2 = {raQ_sec_a, raK_sec_a, rb1_square_secret_b}, and get Q/rs1, K/rs1, [rb1]_s
            1. compute [Z]_s = [Q .* K^T/sqrt(d_k)]_s = Q/rs1 .* K/rs1 * [rs1^2]_s / sqrt(d_k)
            2. generate Zc, compute [Zs]_s = [Z]_s - Zc, exp(Zc)
        */
        LongCiphertext raQ_sec_a, raK_sec_a, rb1_square_secret_b;
        LongCiphertext::recv(io_pack, &raQ_sec_a, party->context);
        LongCiphertext::recv(io_pack, &raK_sec_a, party->context);
        LongCiphertext::recv(io_pack, &rb1_square_secret_b, party->context);
        LongPlaintext raQ_div_rb1_plain = raQ_sec_a.decrypt(party);
        LongPlaintext raK_div_rb1_plain = raK_sec_a.decrypt(party);
        std::vector<double> Q_div_rb1 = raQ_div_rb1_plain.decode(encoder);
        std::vector<double> K_div_rb1 = raK_div_rb1_plain.decode(encoder);
        std::vector<double> eScore_a(batch_size * batch_size);
        random_mat(eScore_a, -10, 0);
        std::vector<double> negScore_a(eScore_a);
        auto sqrt_d_k = sqrt(d_k);
        for (size_t i = 0; i < batch_size * d_k; i++) {
            Q_div_rb1[i] /= ra;
            Q_div_rb1[i] /= sqrt_d_k;
            K_div_rb1[i] /= ra;
        }
        auto temp_Score = matmul(Q_div_rb1, K_div_rb1, batch_size, d_k, batch_size, true);
        for (size_t i = 0; i < batch_size * batch_size; i++) {
            negScore_a[i] = -negScore_a[i];
            eScore_a[i] = exp(eScore_a[i]);
        }
        normalization(temp_Score, batch_size, batch_size);
        LongPlaintext Score_plain(temp_Score, encoder);
        LongCiphertext Score_b_secret_b;
        try {
            Score_b_secret_b = rb1_square_secret_b.multiply_plain(Score_plain, evaluator);
        } catch (std::exception &e) {
#ifdef WARNING
            std::cout << "Zero warning" << std::endl;
#endif
            std::vector<double> temp(Score_plain.len);
            random_mat(temp, -1e-7, 1e-7);
            Score_b_secret_b = LongCiphertext(LongPlaintext(temp, encoder), party);
        }
        LongPlaintext negZc_plain(negScore_a, encoder);
        negZc_plain.mod_switch_to_inplace(Score_b_secret_b.parms_id(), evaluator);
        Score_b_secret_b.add_plain_inplace(negZc_plain, evaluator);
#ifdef SOFTMAX_TIME_TEST
        INIT_TIMER;
        START_TIMER;
#endif
        LongPlaintext eScore_a_plain(eScore_a, encoder);
        LongCiphertext eScore_a_secret_a_(eScore_a_plain, party);
        // send H3 = {Score_b_secret_b, eScore_a_secret_a} to bob
        LongCiphertext::send(io_pack, &Score_b_secret_b);
        LongCiphertext::send(io_pack, &eScore_a_secret_a_);

        /*
            alice receive H4 = {eScore_a_secret_a, eScore_b, raV_sec_a}, and get rb2 * exp(Score) + O,
            Db * exp(Score_b), Rb * V,
            1. compute sum_j (rs2_expScore + O)_ij, (Db * exp(Score_b) * exp(Score_a)) .* Rb * V
            2. (Db * exp(Score_b) * exp(Score_a)) .* Rb * V / (sum_j (rs2_expScore + O)_ij) is
               softmax(QK^T) .* V * (Db ./Rb^T) / rb = output
        */
        LongCiphertext eScore_a_secret_a, raV_sec_a;
        std::vector<double> eScore_b(batch_size * batch_size);
        LongCiphertext::recv(io_pack, &eScore_a_secret_a, party->context);
        recv_mat(io_pack, &eScore_b);
        LongCiphertext::recv(io_pack, &raV_sec_a, party->context);

        LongPlaintext rs2_expScore_plain = eScore_a_secret_a.decrypt(party);
        auto rs2_expScore = rs2_expScore_plain.decode(encoder);

        LongPlaintext Rb_V_plain = raV_sec_a.decrypt(party);
        auto Rb_V = Rb_V_plain.decode(encoder);
        for (size_t i = 0; i < batch_size * d_k; i++)
            Rb_V[i] /= ra;

        std::vector<double> exp_sum(batch_size);
        for (size_t i = 0; i < batch_size; i++) {
            for (j = 0; j < batch_size; j++) {
                exp_sum[i] += rs2_expScore[i * batch_size + j];
            }
        }
        for (i = 0; i < batch_size; i++) {
            for (j = 0; j < batch_size; j++) {
                eScore_b[i * batch_size + j] *= eScore_a[i * batch_size + j];
                eScore_b[i * batch_size + j] /= exp_sum[i];
            }
        }
#ifdef SOFTMAX_TIME_TEST
        STOP_TIMER("attention softmax");
#endif
        auto output = matmul(eScore_b, Rb_V, batch_size, batch_size, d_k);
#ifdef LOG
        char* buf = new char[13];
        sprintf(buf, "Attention-%-2d", head);
        STOP_TIMER(buf)
        delete buf;
#endif
        return output;
    } else {
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif
        /*
        bob: revice H1 = {ra_xa_WIa, ra_xa, ra_WIa, [ra]_a}, and possess: x_b, W_b
        1. compute: rxw_a + rx_a * w_b + rW_a * x_b + [r_a]_a * xw_b = [r_aI]_a , where I stands for  Q,K,V
        2. genereat random num r_b, compute [r_aQ/r_b]_a, [r_aK/r_b]_a, [(r_b)^2]_b
    */
        std::vector<double> ra_xa_WQa(batch_size * d_k),
            ra_xa_WKa(batch_size * d_k),
            ra_xa_WVa(batch_size * d_k),
            ra_xa(batch_size * d_module),
            ra_WQa(d_module * d_k),
            ra_WKa(d_module * d_k),
            ra_WVa(d_module * d_k);
        LongCiphertext ra_secret_a;
        recv_mat(io_pack, &ra_xa_WQa);
        recv_mat(io_pack, &ra_xa_WKa);
        recv_mat(io_pack, &ra_xa_WVa);
        recv_mat(io_pack, &ra_xa);
        recv_mat(io_pack, &ra_WQa);
        recv_mat(io_pack, &ra_WKa);
        recv_mat(io_pack, &ra_WVa);
        LongCiphertext::recv(io_pack, &ra_secret_a, party->context);
        auto cal_raI_A = [](std::vector<double> input_b, std::vector<double> WIb,
                            std::vector<double> ra_xa, std::vector<double> ra_WIa, std::vector<double> ra_xa_WIa,
                            LongCiphertext ra_secret_a,
                            CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator,
                            double scale, size_t d_k) {
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
        LongCiphertext raQ_sec_a = cal_raI_A(input, WQ, ra_xa, ra_WQa, ra_xa_WQa, ra_secret_a,
                                             party, encoder, evaluator, scale, d_k);
        // [r_aK]_A
        LongCiphertext raK_sec_a = cal_raI_A(input, WK, ra_xa, ra_WKa, ra_xa_WKa, ra_secret_a,
                                             party, encoder, evaluator, scale, d_k);
        // [r_aV]_A
        LongCiphertext raV_sec_a = cal_raI_A(input, WV, ra_xa, ra_WVa, ra_xa_WVa, ra_secret_a,
                                             party, encoder, evaluator, scale, d_k);
        double rb1 = dist(gen);
        LongPlaintext div_rb1_plain(1. / rb1, encoder);
        LongCiphertext rb1_square_secret_b(rb1 * rb1, party, encoder);
        div_rb1_plain.mod_switch_to_inplace(raQ_sec_a.parms_id(), evaluator);
        raQ_sec_a.multiply_plain_inplace(div_rb1_plain, evaluator);
        raK_sec_a.multiply_plain_inplace(div_rb1_plain, evaluator);
        // send H2 = {raQ_sec_a, raK_sec_a, rb1_square_secret_b} to alice
        LongCiphertext::send(io_pack, &raQ_sec_a);
        LongCiphertext::send(io_pack, &raK_sec_a);
        LongCiphertext::send(io_pack, &rb1_square_secret_b);

        /*
            bob receive H3, and get Score_b, [exp(Score_c)]_a
            1. generate rb2, Ds, Rs, O randomly, which: sum_j O_ij = 0; Ds is column vector, Rs is row
               vector, they expand to a matrix
            2. compute [rb2*exp(Score) + O]_c = rb2*exp(Score_b)*[exp(Score_a)]_C + O, Db*exp(Score_b),
               Rb*[rcV]_c
            3. output = r / (Db * Rb^T)
        */
        LongCiphertext Score_b_secret_b, eScore_a_secret_a;
        LongCiphertext::recv(io_pack, &Score_b_secret_b, party->context);
        LongCiphertext::recv(io_pack, &eScore_a_secret_a, party->context);
        LongPlaintext eScore_b_plain = Score_b_secret_b.decrypt(party);
        std::vector<double> eScore_b = eScore_b_plain.decode(encoder);
        double rb2 = dist(gen);
        std::vector<double> Db(batch_size);
        random_mat(Db);
        std::vector<double> O = zero_sum(batch_size, batch_size);
        for (size_t i = 0; i < batch_size * batch_size; i++) {
            eScore_b[i] = exp(eScore_b[i]) * rb2;
        }
        LongPlaintext rb2_expZb_plain(eScore_b, encoder);
        try {
            eScore_a_secret_a.multiply_plain_inplace(rb2_expZb_plain, evaluator);
        } catch (std::exception &e) {
#ifdef WARNING
            std::cout << "Zero warning" << std::endl;
#endif
            std::vector<double> temp(eScore_a_secret_a.len);
            random_mat(temp, -1e-7, 1e-7);
            eScore_a_secret_a = LongCiphertext(LongPlaintext(temp, encoder), party);
        }
        LongPlaintext O_plain(O, encoder);
        O_plain.mod_switch_to_inplace(eScore_a_secret_a.parms_id(), evaluator);
        eScore_a_secret_a.add_plain_inplace(O_plain, evaluator);

        for (size_t i = 0; i < batch_size * batch_size; i++)
            eScore_b[i] = eScore_b[i] * Db[i / batch_size] / rb2;
        std::vector<double> Rb(batch_size * d_k);
        random_mat(Rb);
        for (i = 1; i < batch_size; i++)
            for (j = 0; j < d_k; j++)
                Rb[i * d_k + j] = Rb[j];
        LongPlaintext Rb_plain(Rb, encoder);
        Rb_plain.mod_switch_to_inplace(raV_sec_a.parms_id(), evaluator);
        raV_sec_a.multiply_plain_inplace(Rb_plain, evaluator);

        std::vector<double> output(batch_size * d_k);
        for (i = 0; i < batch_size; i++)
            for (j = 0; j < d_k; j++)
                output[i * d_k + j] = rb2 / (Db[i] * Rb[j]);
        // send H4 = {eScore_a_secret_a, eScore_b, raV_sec_a} to alice
        LongCiphertext::send(io_pack, &eScore_a_secret_a);
        send_mat(io_pack, &eScore_b);
        LongCiphertext::send(io_pack, &raV_sec_a);
#ifdef LOG
        char* buf = new char[13];
        sprintf(buf, "Attention-%-2d", head);
        STOP_TIMER(buf)
        delete buf;
#endif
        return output;
    }
}

Multi_Head_Attention::Multi_Head_Attention(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator,
                                           IOPack *io_pack) : Protocol(party, encoder, evaluator, io_pack) {
    attns = new Attention *[n_heads];
    for (int i = 0; i < n_heads; i++) {
        attns[i] = new Attention(party, encoder, evaluator, io_pack, i);
    }
}

Multi_Head_Attention::~Multi_Head_Attention() {
    for (int i = 0; i < n_heads; i++) {
        delete attns[i];
    }
    delete[] attns;
}

LongCiphertext Multi_Head_Attention::forward(const std::vector<double> &input) const {
    std::vector<double> output(input.size());
    size_t d_k = d_module / n_heads;
    size_t i, j;
    for (int h = 0; h < n_heads; h++) {
        auto output_h = attns[h]->forward(input);
        for (i = 0; i < batch_size; i++) {
            for (j = 0; j < d_k; j++) {
                output[i * d_module + h * d_k + j] = output_h[i * d_k + j];
            }
        }
    }
    LongCiphertext output_secret;
    if (party->party == ALICE) {
        LongCiphertext::recv(io_pack, &output_secret, party->context);
    } else {
        LongCiphertext output_secret_b(
            LongPlaintext(output, encoder),
            party);
        LongCiphertext::send(io_pack, &output_secret_b);
    }
    io_pack->io->num_rounds /= 12;
    return output_secret;
}
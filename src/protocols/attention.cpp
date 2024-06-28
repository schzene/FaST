#include "attention.h"
#include "model.h"

Attention::Attention(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator, sci::NetIO *io, int layer, int head_)
    : Protocol(party, encoder, evaluator, io, layer), head(head_) {}

matrix Attention::forward(const matrix &input) const {
    size_t total_comm = 0;
    size_t i, j;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-1, 1);
    if (party->party == sci::ALICE) {
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif
        // alice: possess: x_a, W_a
        double ra = dist(gen);
        matrix ra_xa(batch_size * d_module);
        for (i = 0; i < batch_size * d_module; i++) {
            ra_xa[i] = ra * input[i];
        }
        matrix raWQ(WQ.size()), raWK(WK.size()), raWV(WV.size());
        for (i = 0; i < d_module * d_k; i++) {
            raWQ[i] = ra * WQ[i];
            raWK[i] = ra * WK[i];
            raWV[i] = ra * WV[i];
        }
        matrix ra_xa_WQa = matmul(input, raWQ, batch_size, d_module, d_k);
        matrix ra_xa_WKa = matmul(input, raWK, batch_size, d_module, d_k);
        matrix ra_xa_WVa = matmul(input, raWV, batch_size, d_module, d_k);
        for (i = 0; i < batch_size; i++) {
            for (j = 0; j < d_k; j++) {
                ra_xa_WQa[i * d_k + j] += bQ[j];
                ra_xa_WKa[i * d_k + j] += bK[j];
                ra_xa_WVa[i * d_k + j] += bV[j];
            }
        }
        LongCiphertext ra_secret_a(ra, party, encoder);
        // send H1 = {ra_xa_WIa, ra_xa, ra_WIa, [ra]_a} to bob, where I = Q, K,
        // V
        send_mat(io, &ra_xa_WQa);
        send_mat(io, &ra_xa_WKa);
        send_mat(io, &ra_xa_WVa);
        send_mat(io, &ra_xa);
        send_mat(io, &WQ);
        send_mat(io, &WK);
        send_mat(io, &WV);
        LongCiphertext::send(io, &ra_secret_a);

        /*
            alice receive H2 = {raQ_sec_a, raK_sec_a, rb1_square_secret_b}, and
           get Q/rs1, K/rs1, [rb1]_s
            1. compute [Z]_s = [Q .* K^T/sqrt(d_k)]_s = Q/rs1 .* K/rs1 *
           [rs1^2]_s / sqrt(d_k)
            2. generate Zc, compute [Zs]_s = [Z]_s - Zc, exp(Zc)
        */
        LongCiphertext raQ_sec_a, raK_sec_a, rb1_square_secret_b;
        LongCiphertext::recv(io, &raQ_sec_a, party->context);
        LongCiphertext::recv(io, &raK_sec_a, party->context);
        LongCiphertext::recv(io, &rb1_square_secret_b, party->context);
        LongPlaintext raQ_div_rb1_plain = raQ_sec_a.decrypt(party);
        LongPlaintext raK_div_rb1_plain = raK_sec_a.decrypt(party);
        matrix Q_div_rb1 = raQ_div_rb1_plain.decode(encoder);
        matrix K_div_rb1 = raK_div_rb1_plain.decode(encoder);
        matrix eScore_a(batch_size * batch_size);
        random_mat(eScore_a, -10, 0);
        matrix negScore_a(eScore_a);
        auto sqrt_d_k = sqrt(d_k);
        for (size_t i = 0; i < batch_size * d_k; i++) {
            Q_div_rb1[i] /= ra;
            Q_div_rb1[i] /= sqrt_d_k;
            K_div_rb1[i] /= ra;
        }
        matrix temp_Score = matmul(Q_div_rb1, K_div_rb1, batch_size, d_k, batch_size, true);
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
            std::cout << "Zero warning\n";
#endif
            matrix temp(Score_plain.len);
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
        LongCiphertext::send(io, &Score_b_secret_b);
        LongCiphertext::send(io, &eScore_a_secret_a_);

        /*
            alice receive H4 = {eScore_a_secret_a, eScore_b, raV_sec_a}, and get
           rb2 * exp(Score) + O, Db * exp(Score_b), Rb * V,
            1. compute sum_j (rs2_expScore + O)_ij, (Db * exp(Score_b) *
           exp(Score_a)) .* Rb * V
            2. (Db * exp(Score_b) * exp(Score_a)) .* Rb * V / (sum_j
           (rs2_expScore + O)_ij) is softmax(QK^T) .* V * (Db ./Rb^T) / rb =
           output
        */
        LongCiphertext eScore_a_secret_a, raV_sec_a;
        matrix eScore_b(batch_size * batch_size);
        LongCiphertext::recv(io, &eScore_a_secret_a, party->context);
        recv_mat(io, &eScore_b);
        LongCiphertext::recv(io, &raV_sec_a, party->context);

        LongPlaintext rs2_expScore_plain = eScore_a_secret_a.decrypt(party);
        matrix rs2_expScore = rs2_expScore_plain.decode(encoder);

        LongPlaintext Rb_V_plain = raV_sec_a.decrypt(party);
        matrix Rb_V = Rb_V_plain.decode(encoder);
        for (size_t i = 0; i < batch_size * d_k; i++) {
            Rb_V[i] /= ra;
        }

        matrix exp_sum(batch_size);
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
        matrix output = matmul(eScore_b, Rb_V, batch_size, batch_size, d_k);
#ifdef LOG
        char *buf = new char[13];
        sprintf(buf, "Attention-%-2d", head);

        STOP_TIMER(buf)
        total_comm += io->counter;
        printf("%s Send data %ld Bytes. \n", buf, total_comm);
        delete[] buf;
#endif
        return output;
    } else {
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif
        /*
        bob: revice H1 = {ra_xa_WIa, ra_xa, ra_WIa, [ra]_a}, and possess: x_b,
        W_b
        1. compute: rxw_a + rx_a * w_b + rW_a * x_b + [r_a]_a * xw_b = [r_aI]_a
        , where I stands for  Q,K,V
        2. genereat random num r_b, compute [r_aQ/r_b]_a, [r_aK/r_b]_a,
        [(r_b)^2]_b
    */

        // 1. computate r_aQ /r_aK/r_aV
        matrix ra_xa_WQa(batch_size * d_k), ra_xa_WKa(batch_size * d_k), ra_xa_WVa(batch_size * d_k),
            ra_xa(batch_size * d_module), ra_WQa(d_module * d_k), ra_WKa(d_module * d_k), ra_WVa(d_module * d_k);
        LongCiphertext ra_secret_a;
        recv_mat(io, &ra_xa_WQa);
        recv_mat(io, &ra_xa_WKa);
        recv_mat(io, &ra_xa_WVa);
        recv_mat(io, &ra_xa);
        recv_mat(io, &ra_WQa);
        recv_mat(io, &ra_WKa);
        recv_mat(io, &ra_WVa);
        LongCiphertext::recv(io, &ra_secret_a, party->context);
        auto cal_raI_A = [&input, &ra_xa, &ra_secret_a, this](matrix WIb, matrix bI, matrix ra_WIa, matrix ra_xa_WIa) {
            matrix xbWI_b = matmul(input, WIb, batch_size, d_module, d_k);
            LongPlaintext xbWI_b_plain(xbWI_b, encoder);
            LongCiphertext raI_secret_a = ra_secret_a.multiply_plain(xbWI_b_plain, evaluator); // element-wise matmul

            matrix temp_raI(batch_size * d_k);
            matrix temp_raI1 = matmul(ra_xa, WIb, batch_size, d_module, d_k);
            matrix temp_raI2 = matmul(input, ra_WIa, batch_size, d_module, d_k);
            for (size_t i = 0; i < batch_size; i++) {
                for (size_t j = 0; j < d_k; j++) {
                    temp_raI[i * d_k + j] =
                        ra_xa_WIa[i * d_k + j] + temp_raI1[i * d_k + j] + temp_raI2[i * d_k + j] + bI[j];
                }
            }
            LongPlaintext temp_raI_plain(temp_raI, encoder);
            temp_raI_plain.mod_switch_to_inplace(raI_secret_a.parms_id(), evaluator);
            raI_secret_a.add_plain_inplace(temp_raI_plain, evaluator);
            return raI_secret_a;
        };
        // [r_aQ]_A
        LongCiphertext raQ_sec_a = cal_raI_A(WQ, bQ, ra_WQa, ra_xa_WQa);
        // [r_aK]_A
        LongCiphertext raK_sec_a = cal_raI_A(WK, bK, ra_WKa, ra_xa_WKa);
        // [r_aV]_A
        LongCiphertext raV_sec_a = cal_raI_A(WV, bV, ra_WVa, ra_xa_WVa);

        // 2. generate 1 / rb1
        double rb1 = dist(gen);
        LongPlaintext div_rb1_plain(1. / rb1, encoder);
        LongCiphertext rb1_square_secret_b(rb1 * rb1, party, encoder);
        div_rb1_plain.mod_switch_to_inplace(raQ_sec_a.parms_id(), evaluator);
        raQ_sec_a.multiply_plain_inplace(div_rb1_plain, evaluator);
        raK_sec_a.multiply_plain_inplace(div_rb1_plain, evaluator);

        // send H2 = {raQ_sec_a, raK_sec_a, rb1_square_secret_b} to alice
        LongCiphertext::send(io, &raQ_sec_a);
        LongCiphertext::send(io, &raK_sec_a);
        LongCiphertext::send(io, &rb1_square_secret_b);

        /*
            bob receive H3, and get Score_b, [exp(Score_c)]_a
            1. generate rb2, Ds, Rs, O randomly, which: sum_j O_ij = 0; Ds is
           column vector, Rs is row vector, they expand to a matrix
            2. compute [rb2*exp(Score) + O]_c =
           rb2*exp(Score_b)*[exp(Score_a)]_C + O, Db*exp(Score_b), Rb*[rcV]_c
            3. output = r / (Db * Rb^T)
        */
        LongCiphertext Score_b_secret_b, eScore_a_secret_a;
        LongCiphertext::recv(io, &Score_b_secret_b, party->context);
        LongCiphertext::recv(io, &eScore_a_secret_a, party->context);
        LongPlaintext eScore_b_plain = Score_b_secret_b.decrypt(party);
        matrix eScore_b = eScore_b_plain.decode(encoder);
        double rb2 = dist(gen);
        matrix Db(batch_size);
        random_mat(Db);
        matrix O = zero_sum(batch_size, batch_size);
        for (size_t i = 0; i < batch_size * batch_size; i++) {
            eScore_b[i] = exp(eScore_b[i]) * rb2;
        }
        LongPlaintext rb2_expZb_plain(eScore_b, encoder);
        try {
            eScore_a_secret_a.multiply_plain_inplace(rb2_expZb_plain, evaluator);
        } catch (std::exception &e) {
#ifdef WARNING
            std::cout << "Zero warning\n";
#endif
            matrix temp(eScore_a_secret_a.len);
            random_mat(temp, -1e-7, 1e-7);
            eScore_a_secret_a = LongCiphertext(LongPlaintext(temp, encoder), party);
        }
        LongPlaintext O_plain(O, encoder);
        O_plain.mod_switch_to_inplace(eScore_a_secret_a.parms_id(), evaluator);
        eScore_a_secret_a.add_plain_inplace(O_plain, evaluator);

        for (size_t i = 0; i < batch_size * batch_size; i++) {
            eScore_b[i] = eScore_b[i] * Db[i / batch_size] / rb2;
        }
        matrix Rb(batch_size * d_k);
        random_mat(Rb);
        for (i = 1; i < batch_size; i++) {
            for (j = 0; j < d_k; j++) {
                Rb[i * d_k + j] = Rb[j];
            }
        }
        LongPlaintext Rb_plain(Rb, encoder);
        Rb_plain.mod_switch_to_inplace(raV_sec_a.parms_id(), evaluator);
        raV_sec_a.multiply_plain_inplace(Rb_plain, evaluator);

        matrix output(batch_size * d_k);
        for (i = 0; i < batch_size; i++) {
            for (j = 0; j < d_k; j++) {
                output[i * d_k + j] = rb2 / (Db[i] * Rb[j]);
            }
        }
        // send H4 = {eScore_a_secret_a, eScore_b, raV_sec_a} to alice
        LongCiphertext::send(io, &eScore_a_secret_a);
        send_mat(io, &eScore_b);
        LongCiphertext::send(io, &raV_sec_a);
#ifdef LOG
        char *buf = new char[13];
        sprintf(buf, "Attention-%-2d", head);
        STOP_TIMER(buf)
        total_comm += io->counter;
        printf("%s Send data %ld Bytes. \n", buf, total_comm);
        delete[] buf;
#endif
        return output;
    }
}

Multi_Head_Attention::Multi_Head_Attention(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator, sci::NetIO *io,
                                           int layer)
    : Protocol(party, encoder, evaluator, io, layer) {
    attns = new Attention *[n_heads];
    string layer_str = std::to_string(layer),
           dir_path = party->party == sci::ALICE ? "/data/BOLT/bolt/prune/mrpc/alice_weights_txt/"
                                                 : "/data/BOLT/bolt/prune/mrpc/bob_weights_txt/",
           WQ_file = replace("bert.encoder.layer.LAYER.attention.self.query.weight.txt", "LAYER", layer_str),
           WK_file = replace("bert.encoder.layer.LAYER.attention.self.key.weight.txt", "LAYER", layer_str),
           WV_file = replace("bert.encoder.layer.LAYER.attention.self.value.weight.txt", "LAYER", layer_str),
           bQ_file = replace("bert.encoder.layer.LAYER.attention.self.query.bias.txt", "LAYER", layer_str),
           bK_file = replace("bert.encoder.layer.LAYER.attention.self.key.bias.txt", "LAYER", layer_str),
           bV_file = replace("bert.encoder.layer.LAYER.attention.self.value.bias.txt", "LAYER", layer_str);
    matrix allWQ, allWK, allWV, bQ, bK, bV;
    load_mat(allWQ, dir_path + WQ_file);
    load_mat(allWK, dir_path + WK_file);
    load_mat(allWV, dir_path + WV_file);
    load_mat(bQ, dir_path + bQ_file);
    load_mat(bK, dir_path + bK_file);
    load_mat(bV, dir_path + bV_file);
    size_t weight_size = d_module * d_k;
    size_t bias_size = d_k;
    for (int i = 0; i < n_heads; i++) {
        attns[i] = new Attention(party, encoder, evaluator, io, layer, i);
        attns[i]->WQ = matrix(allWQ.begin() + i * weight_size, allWQ.begin() + (i + 1) * weight_size);
        attns[i]->WK = matrix(allWK.begin() + i * weight_size, allWK.begin() + (i + 1) * weight_size);
        attns[i]->WV = matrix(allWV.begin() + i * weight_size, allWV.begin() + (i + 1) * weight_size);
        attns[i]->bQ = matrix(bQ.begin() + i * bias_size, bQ.begin() + (i + 1) * bias_size);
        attns[i]->bK = matrix(bK.begin() + i * bias_size, bK.begin() + (i + 1) * bias_size);
        attns[i]->bV = matrix(bV.begin() + i * bias_size, bV.begin() + (i + 1) * bias_size);
    }
}

Multi_Head_Attention::~Multi_Head_Attention() {
    for (int i = 0; i < n_heads; i++) {
        delete attns[i];
    }
    delete[] attns;
}

LongCiphertext Multi_Head_Attention::forward(const matrix &input) const {
    matrix output(batch_size * d_module);

    size_t i, j;
    for (int h = 0; h < n_heads; h++) {
        matrix output_h = attns[h]->forward(input);
        for (i = 0; i < batch_size; i++) {
            for (j = 0; j < d_k; j++) {
                output[i * d_module + h * d_k + j] = output_h[i * d_k + j];
            }
        }
    }

    LongCiphertext output_secret;
    if (party->party == sci::ALICE) {
        LongCiphertext::recv(io, &output_secret, party->context);
        LongPlaintext output_plain = LongPlaintext(output, encoder);
        output_secret.multiply_plain_inplace(output_plain, evaluator);
    } else {
        LongCiphertext output_secret_b(LongPlaintext(output, encoder), party);
        LongCiphertext::send(io, &output_secret_b);
    }
    // io->io->num_rounds /= 12;
    return output_secret;
}
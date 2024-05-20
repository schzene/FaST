#include "layer-norm.h"
LongCiphertext LayerNorm::forward(const LongCiphertext &attn, const matrix &input) const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-1, 1);
    size_t i, j;

    if (party->party == ALICE) {
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif
        double ha1 = dist(gen), ha2 = dist(gen);
        matrix ha1_xa(input.size());
        for (i = 0; i < batch_size * d_module; i++) {
            ha1_xa[i] = ha1 * input[i];
        }
        LongCiphertext ha2_div_ha1_secret_a(ha2 / ha1, party, encoder);
        LongPlaintext ha2_plain(ha2, encoder);
        LongCiphertext ha2_secret_a(ha2_plain, party);
        LongCiphertext attn_ha2_b = attn.multiply_plain(ha2_plain, evaluator);
        // send H1 = {ha1_xa, ha2_div_hc1_secret_a, ha2_secret_a, attn_ha2_b} to bob
        send_mat(io_pack, &ha1_xa);
        LongCiphertext::send(io_pack, &ha2_div_ha1_secret_a);
        LongCiphertext::send(io_pack, &ha2_secret_a);
        LongCiphertext::send(io_pack, &attn_ha2_b);

        /*
            alice receive H2, and get x * gb
            1. compute mean(x * gb) = gb * \mu, standard_deviation(x * gb, gb * \mu) = sigma * gs
            2. generate kc, compute tmp1 = (x * gb - gb * \mu) * kc = (x - \mu)*gb*kc, tmp2_secret_c = [1 / (sigma * gs * ka)]_c
        */
        LongCiphertext xha1_secret_a;
        LongCiphertext::recv(io_pack, &xha1_secret_a, party->context);
        auto xgb_plain = xha1_secret_a.decrypt(party);
        auto xgb = xgb_plain.decode(encoder);
        for (i = 0; i < batch_size * d_module; i++) {
            xgb[i] /= ha2;
        }
        double ka = dist(gen);
        auto mu_gb = mean(xgb, batch_size, d_module);
        auto sigma_gb = standard_deviation(xgb, mu_gb, batch_size, d_module);
        matrix div_sigma_gb(batch_size * d_module);
        matrix tmp1(batch_size * d_module);
        for (i = 0; i < batch_size; i++) {
            for (j = 0; j < d_module; j++) {
                tmp1[i * d_module + j] = (xgb[i * d_module + j] - mu_gb[i]) * ka;
                div_sigma_gb[i * d_module + j] = 1 / (sigma_gb[i] * ka);
            }
        }
        LongPlaintext div_sigma_gb_plain(div_sigma_gb, encoder);
        LongCiphertext tmp2_secret_a(div_sigma_gb_plain, party);
        // send H3 = {tmp1, tmp2_secret_a} to bob
        send_mat(io_pack, &tmp1);
        LongCiphertext::send(io_pack, &tmp2_secret_a);
#ifdef LOG
        STOP_TIMER("Layer Norm")
#endif
        return LongCiphertext();
    } else {
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif
        /*
            bob receive H1, and get ha1_xa, ha2_div_hc1_secret_a, ha2_secret_a, attn_ha2
            1. compute attn_ha2 + ha1_xa *  [ha2/ha1]_c + xb*[ha2]_c = [x * ha2]_c
            2. generate gs, coupute [x * ha2]_c * gs = [x * ha2 * gs]_c
        */
        matrix ha1_xa(batch_size * d_module);
        LongCiphertext ha2_div_ha1_secret_a, ha2_secret_a, attn_ha2_b;
        recv_mat(io_pack, &ha1_xa);
        LongCiphertext::recv(io_pack, &ha2_div_ha1_secret_a, party->context);
        LongCiphertext::recv(io_pack, &ha2_secret_a, party->context);
        LongCiphertext::recv(io_pack, &attn_ha2_b, party->context);

        auto attn_ha2_plain = attn_ha2_b.decrypt(party);
        LongPlaintext input_b_plain(input, encoder);
        LongCiphertext xha1_secret_a = ha2_secret_a.multiply_plain(input_b_plain, evaluator);
        attn_ha2_plain.mod_switch_to_inplace(xha1_secret_a.parms_id(), evaluator);
        xha1_secret_a.add_plain_inplace(attn_ha2_plain, evaluator);

        LongPlaintext ha1_xc_plain(ha1_xa, encoder);
        ha2_div_ha1_secret_a.multiply_plain_inplace(ha1_xc_plain, evaluator);
        ha2_div_ha1_secret_a.mod_switch_to_inplace(xha1_secret_a.parms_id(), evaluator);
        xha1_secret_a.add_inplace(ha2_div_ha1_secret_a, evaluator);

        double gb = dist(gen);
        gb = 1;
        LongPlaintext gb_plain(gb, encoder);
        gb_plain.mod_switch_to_inplace(xha1_secret_a.parms_id(), evaluator);
        xha1_secret_a.multiply_plain_inplace(gb_plain, evaluator);
        // send H2 = {xha1_secret_a} to alice;
        LongCiphertext::send(io_pack, &xha1_secret_a);

        /*
            bob receive H3
            1. compute tmp1 * tmp2 = output;
        */
        matrix tmp1(batch_size * d_module);
        LongCiphertext tmp2_secret_a;
        recv_mat(io_pack, &tmp1);
        LongCiphertext::recv(io_pack, &tmp2_secret_a, party->context);
        matrix gamma(batch_size * d_module);
        matrix beta(batch_size * d_module);
        random_mat(gamma);
        random_mat(beta);
        for (i = 0; i < batch_size * d_module; i++) {
            tmp1[i] *= gamma[i];
        }
        LongPlaintext gamma_tmp1_plain(tmp1, encoder), beta_plain(beta, encoder);
        LongCiphertext ln_secret_a = tmp2_secret_a.multiply_plain(gamma_tmp1_plain, evaluator);
        beta_plain.mod_switch_to_inplace(ln_secret_a.parms_id(), evaluator);
        ln_secret_a.add_plain_inplace(beta_plain, evaluator);
#ifdef LOG
        STOP_TIMER("Layer Norm")
#endif
        return ln_secret_a;
    }
}
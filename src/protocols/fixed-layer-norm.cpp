#include "fixed-layer-norm.h"

FixArray mean(const FixArray &input, size_t row, size_t column) {
    FixArray result(input.party, row, input.signed_, input.ell, input.s);
    size_t i, j;
    for (i = 0; i < row; i++) {
        for (j = 0; j < column; j++) {
            result.data[i] += input.data[i * column + j];
        }
        result.data[i] /= column;
    }
    return result;
}

FixArray standard_deviation(const FixArray &input, const FixArray &means,
                            size_t row, size_t column) {
    FixArray result(input.party, row, input.signed_, input.ell, input.s);
    size_t i, j;
    for (i = 0; i < row; i++) {
        for (j = 0; j < column; j++) {
            result.data[i] += (input.data[i * column + j] - means.data[i]) *
                              (input.data[i * column + j] - means.data[i]);
        }
        result.data[i] /= column;
        result.data[i] = static_cast<uint64_t>(sqrt(result.data[i]));
    }
    return result;
}

BFVLongCiphertext FixedLayerNorm::forward(const BFVLongCiphertext &attn,
                                          const bfv_matrix &input) const {
    sci::PRG128 prg;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-1, 1);
    size_t i, j;

    if (party->party == sci::ALICE) {
        // #ifdef LOG
        //         INIT_TIMER
        //         START_TIMER
        // #endif
        const double ha1 = dist(gen), ha2 = dist(gen), ka = dist(gen);
        const double ha2_div_ha1 = ha2 / ha1, div_ha2 = 1 / ha2;

        const uint64_t fix_ha1 =
            sci::neg_mod(static_cast<int64_t>(ha1 * (1ULL << DEFAULT_BITWIDTH)),
                         DEFAULT_ELL);
        const uint64_t fix_ha2 =
            sci::neg_mod(static_cast<int64_t>(ha2 * (1ULL << DEFAULT_BITWIDTH)),
                         DEFAULT_ELL);
        const uint64_t fix_h2_div_h1 = sci::neg_mod(
            static_cast<int64_t>(ha2_div_ha1 * (1ULL << DEFAULT_BITWIDTH)),
            DEFAULT_ELL);
        const uint64_t fix_div_ha2 = sci::neg_mod(
            static_cast<int64_t>(div_ha2 * (1ULL << DEFAULT_BITWIDTH)),
            DEFAULT_ELL);

        FixArray fix_input =
            fixop->input(party->party, input.size(), input.data());
        BFVLongPlaintext ha2_plain(parm, fix_ha2);
        BFVLongCiphertext ha2_div_ha1_secret_a(parm, fix_h2_div_h1, party),
            ha2_secret_a(ha2_plain, party), xha1_secret_a;

        FixArray fixed_ha1_xa = fixop->mul(fix_input, fix_ha1);
        for (i = 0; i < fixed_ha1_xa.size; i++) {
            fixed_ha1_xa.data[i] >>= DEFAULT_BITWIDTH;
        }
        fixed_ha1_xa.party = sci::PUBLIC;
        BFVLongCiphertext attn_ha2_b =
            attn.multiply_plain(ha2_plain, party->parm->evaluator);
        // send H1 = {ha1_xa, ha2_div_hc1_secret_a, ha2_secret_a, attn_ha2_b} to
        // bob
        fix_public->send_fix_array(fixed_ha1_xa);
        BFVLongCiphertext::send(fix_public->iopack->io, &ha2_div_ha1_secret_a);
        BFVLongCiphertext::send(fix_public->iopack->io, &ha2_secret_a);
        BFVLongCiphertext::send(fix_public->iopack->io, &attn_ha2_b);

        /*
            alice receive H2, and get x * gb
            1. compute mean(x * gb) = gb * \mu, standard_deviation(x * gb, gb *
           \mu) = sigma * gs
            2. generate kc, compute tmp1 = (x * gb - gb * \mu) * kc = (x -
           \mu)*gb*kc, tmp2_secret_c = [1 / (sigma * gs * ka)]_c
        */
        BFVLongCiphertext::recv(fix_public->iopack->io, &xha1_secret_a,
                                parm->context);
        BFVLongPlaintext ha2xgb_plain = xha1_secret_a.decrypt(party);
        bfv_matrix ha2xgb = ha2xgb_plain.decode(parm);
        FixArray fix_ha2xgb =
            fixop->input(sci::PUBLIC, ha2xgb.size(), ha2xgb.data());
        FixArray fix_xgb = fixop->mul(fix_ha2xgb, fix_div_ha2);
        for (i = 0; i < fix_xgb.size; i++) {
            fix_xgb.data[i] >>= DEFAULT_BITWIDTH;
        }
        fix_xgb.party = sci::ALICE;
        FixArray mu_gb = mean(fix_xgb, batch_size, d_module);
        FixArray sigma_gb =
            standard_deviation(fix_xgb, mu_gb, batch_size, d_module);
        bfv_matrix div_sigma_gb(batch_size * d_module);
        bfv_matrix tmp1(batch_size * d_module);
        for (i = 0; i < batch_size; i++) {
            for (j = 0; j < d_module; j++) {
                int64_t tmp0 =
                    (fix_xgb.data[i * d_module + j] - mu_gb.data[i]) *
                        int64_t(ka * (1ULL << DEFAULT_BITWIDTH)) >>
                    DEFAULT_BITWIDTH;
                tmp1[i * d_module + j] =
                    sci::neg_mod(tmp0, 1ULL << DEFAULT_BITWIDTH);
                double tmp2 = ka * double(sigma_gb.data[i]) /
                              (1ULL << DEFAULT_BITWIDTH),
                       tmp3 = 1 / tmp2;
                div_sigma_gb[i * d_module + j] =
                    sci::neg_mod(int64_t(tmp3 * (1ULL << DEFAULT_BITWIDTH)),
                                 DEFAULT_BITWIDTH);
            }
        }
        BFVLongPlaintext div_sigma_gb_plain(parm, div_sigma_gb);
        BFVLongCiphertext tmp2_secret_a(div_sigma_gb_plain, party);
        // send H3 = {tmp1, tmp2_secret_a} to bob
        FixArray fix_tmp1 =
            fix_public->input(sci::PUBLIC, tmp1.size(), tmp1.data());
        fixop->send_fix_array(fix_tmp1);
        BFVLongCiphertext::send(fix_public->iopack->io, &tmp2_secret_a);
    } else {
        const double gb = dist(gen);
        const uint64_t fix_gb = sci::neg_mod(
            static_cast<int64_t>(gb * (1ULL << DEFAULT_BITWIDTH)), DEFAULT_ELL);
        bfv_matrix gamma(batch_size * d_module), beta(batch_size * d_module);
        FixArray ha1_xa, tmp1;
        BFVLongPlaintext input_b_plain(parm, input), gb_plain(parm, fix_gb);
        BFVLongCiphertext ha2_div_ha1_secret_a, ha2_secret_a, attn_ha2_b,
            tmp2_secret_a;
        random_bfv_mat(gamma);
        random_bfv_mat(beta);

        fix_public->recv_fix_array(ha1_xa);
        BFVLongCiphertext::recv(fix_public->iopack->io, &ha2_div_ha1_secret_a,
                                parm->context);
        BFVLongCiphertext::recv(fix_public->iopack->io, &ha2_secret_a,
                                parm->context);
        BFVLongCiphertext::recv(fix_public->iopack->io, &attn_ha2_b,
                                parm->context);
        BFVLongPlaintext attn_ha2_plain = attn_ha2_b.decrypt(party);
        BFVLongCiphertext xha1_secret_a =
            ha2_secret_a.multiply_plain(input_b_plain, parm->evaluator);
        // attn_ha2_plain.mod_switch_to_inplace(xha1_secret_a.parms_id(),
        // parm->evaluator);
        xha1_secret_a.add_plain_inplace(attn_ha2_plain, parm->evaluator);

        BFVLongPlaintext ha1_xc_plain(parm, ha1_xa.data, ha1_xa.size);
        ha2_div_ha1_secret_a.multiply_plain_inplace(ha1_xc_plain,
                                                    parm->evaluator);
        // ha2_div_ha1_secret_a.mod_switch_to_inplace(xha1_secret_a.parms_id(),
        // parm->evaluator);
        xha1_secret_a.add_inplace(ha2_div_ha1_secret_a, parm->evaluator);
        // gb_plain.mod_switch_to_inplace(xha1_secret_a.parms_id(),
        // parm->evaluator);
        xha1_secret_a.multiply_plain_inplace(gb_plain, parm->evaluator);
        // send H2 = {xha1_secret_a} to alice;
        BFVLongCiphertext::send(fix_public->iopack->io, &xha1_secret_a);

        /*
            bob receive H3
            1. compute tmp1 * tmp2 = output;
        */
        fix_public->recv_fix_array(tmp1);
        BFVLongCiphertext::recv(fix_public->iopack->io, &tmp2_secret_a,
                                parm->context);
        for (i = 0; i < batch_size * d_module; i++) {
            tmp1.data[i] *= gamma[i];
        }
        BFVLongPlaintext gamma_tmp1_plain(parm, tmp1.data, tmp1.size),
            beta_plain(parm, beta);
        BFVLongCiphertext ln_secret_a =
            tmp2_secret_a.multiply_plain(gamma_tmp1_plain, parm->evaluator);
        // beta_plain.mod_switch_to_inplace(ln_secret_a.parms_id(),
        // parm->evaluator);
        ln_secret_a.add_plain_inplace(beta_plain, parm->evaluator);
        return ln_secret_a;
    }
    return BFVLongCiphertext();
}
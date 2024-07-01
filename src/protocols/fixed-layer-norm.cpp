#include "fixed-layer-norm.h"
#include "FixedPoint/fixed-point.h"
#include "Utils/constants.h"
#include "model.h"
#include "protocols/fixed-protocol.h"
#include "utils/he-bfv.h"
#include <cstdint>

FixedLayerNorm::FixedLayerNorm(BFVKey *party, BFVParm *parm, sci::NetIO *io, FPMath *fpmath, FPMath *fpmath_public,
                               Conversion *conv, int layer, bool _before_attn)
    : FixedProtocol(party, parm, io, fpmath, fpmath_public, conv, layer), before_attn(_before_attn) {
    string layer_str = std::to_string(layer),
           gamma_file =
               before_attn
                   ? replace("bert.encoder.layer.LAYER.attention.output.LayerNorm.weight.txt", "LAYER", layer_str)
                   : replace("bert.encoder.layer.LAYER.output.LayerNorm.weight.txt", "LAYER", layer_str),
           beta_file = before_attn
                           ? replace("bert.encoder.layer.LAYER.attention.output.LayerNorm.bias.txt", "LAYER", layer_str)
                           : replace("bert.encoder.layer.LAYER.output.LayerNorm.bias.txt", "LAYER", layer_str);
    if (party->party == sci::BOB) {
        load_bfv_mat(gamma, dir_path + gamma_file);
        load_bfv_mat(beta, dir_path + beta_file);
    }
}

BFVLongCiphertext FixedLayerNorm::forward(const BFVLongCiphertext &attn, const bfv_matrix &input) const {

    sci::PRG128 prg;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-1, 1);
    size_t i, j;

    uint64_t *x = new uint64_t[input.size()];
    for (size_t i = 0; i < input.size(); i++) {
        x[i] = input[i];
    }

    if (party->party == sci::ALICE) {
        /*
        Alice generate ha
        compute xa_ha,  [ha], attn_sec_b_ ha
        */

        double ha = dist(gen);
        uint64_t *prime_ha_xa = new uint64_t[batch_size * d_module];
        uint64_t *prime_ha = new uint64_t[batch_size * d_module];
        FixArray fix_ha = fpmath->fix->input(
            sci::ALICE, batch_size * d_module,
            (sci::neg_mod(static_cast<int64_t>(ha * (1ULL << (DEFAULT_SCALE))), 1ULL << (DEFAULT_ELL))), true,
            DEFAULT_ELL, DEFAULT_SCALE);

        FixArray fix_div_ha = fpmath->fix->input(
            sci::ALICE, batch_size * d_module,
            (sci::neg_mod(static_cast<int64_t>((1.0 / ha) * (1ULL << (DEFAULT_SCALE))), 1ULL << (DEFAULT_ELL))), true,
            DEFAULT_ELL, DEFAULT_SCALE);
        FixArray fix_xa = fpmath->fix->input(sci::ALICE, batch_size * d_module, x, true, DEFAULT_ELL, DEFAULT_SCALE);

#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif
        fix_ha.party = sci::PUBLIC; // just to make the mul useful.
        FixArray fix_ha_xa = fpmath->fix->mul(fix_xa, fix_ha, DEFAULT_ELL);
        fix_ha_xa = fpmath->fix->location_truncation(fix_ha_xa, DEFAULT_SCALE);
        conv->Ring_to_Prime(fix_ha_xa.data, prime_ha_xa, batch_size * d_module, DEFAULT_ELL, parm->plain_mod);
        conv->Ring_to_Prime(fix_ha.data, prime_ha, batch_size * d_module, DEFAULT_ELL, parm->plain_mod);
        BFVLongPlaintext ha_plain(parm, prime_ha_xa, batch_size * d_module);
        BFVLongCiphertext ha_secret_a(ha_plain, party);
        BFVLongCiphertext attn_ha_secret_b = attn.multiply_plain(ha_plain, parm->evaluator);
        attn_ha_secret_b.mod_switch_to_next_inplace(parm->evaluator);

        // Alice : send H1 = {ha_xa, ha_secret_a, attn_ha_secret_b} to bob
        io->send_data(prime_ha_xa, batch_size * d_module * sizeof(uint64_t));
        send_mat(io, fix_ha_xa.data, fix_ha_xa.size);
        BFVLongCiphertext::send(io, &ha_secret_a);
        BFVLongCiphertext::send(io, &attn_ha_secret_b);
        /*
        alice receive H2, and get x * gb
        1. compute mean(x * gb) = gb * \mu, standard_deviation(x * gb, gb * \mu)
        = sigma * gs
        2. generate kc, compute tmp1 = (x * gb - gb * \mu) * kc = (x -
        \mu)*gb*kc, tmp2_secret_c = [1 / (sigma * gs * ka)]_c
        */
        // Alice: alice receive message and get x * gb;
        BFVLongCiphertext xb_ha_secret_a;
        BFVLongCiphertext::recv(io, &xb_ha_secret_a, party->parm->context);
        BFVLongPlaintext xgb_ha_plain = xb_ha_secret_a.decrypt(party);
        bfv_matrix x_gb_ha_matrix = xgb_ha_plain.decode(party->parm); // something wrong here

        FixArray fix_x_gb(sci::ALICE, batch_size * d_module, true, DEFAULT_ELL, DEFAULT_SCALE);
        uint64_t *x_gb_ha_prime = new uint64_t[batch_size * d_module];
        uint64_t *x_gb_ha_ring = new uint64_t[batch_size * d_module];

        for (size_t i = 0; i < batch_size * d_module; i++) {
            x_gb_ha_prime[i] = x_gb_ha_matrix[i];
        }
        conv->Prime_to_Ring(x_gb_ha_prime, x_gb_ha_ring, batch_size * d_module, DEFAULT_ELL, party->parm->plain_mod,
                            DEFAULT_SCALE, DEFAULT_SCALE, fpmath_public);

        fix_x_gb =
            fpmath->fix->input(sci::ALICE, batch_size * d_module, x_gb_ha_ring, true, DEFAULT_ELL, DEFAULT_SCALE * 2);
        fix_x_gb = fpmath->fix->location_truncation(fix_x_gb, DEFAULT_SCALE);
        fix_div_ha.party = sci::PUBLIC;
        fix_x_gb = fpmath->fix->mul(fix_x_gb, fix_div_ha, DEFAULT_ELL);
        fix_x_gb = fpmath->fix->location_truncation(fix_x_gb, DEFAULT_SCALE);

        vector<FixArray> vec_x_gb;

        for (size_t i = 0; i < batch_size; i++) {
            vec_x_gb.push_back(fpmath->fix->input(fix_x_gb.party, d_module, &fix_x_gb.data[i * d_module],
                                                  fix_x_gb.signed_, fix_x_gb.ell, fix_x_gb.s));
        }

        vector<FixArray> fix_mean_g = fpmath->mean(vec_x_gb); // dim:  batch_size * 1

        vector<FixArray> delta_gb = fpmath->standard_deviation(vec_x_gb, fix_mean_g); // delta -1/2

        double ka = dist(gen);
        FixArray fix_ka = fpmath->fix->input(
            sci::ALICE, 1, sci::neg_mod(static_cast<int64_t>(ka * (1ULL << (DEFAULT_SCALE))), 1ULL << (DEFAULT_ELL)),
            true, DEFAULT_ELL, DEFAULT_SCALE);
        FixArray fix_div_ka = fpmath->fix->input(
            sci::ALICE, 1,
            sci::neg_mod(static_cast<int64_t>((1.0 / ka) * (1ULL << (DEFAULT_SCALE))), 1ULL << (DEFAULT_ELL)), true,
            DEFAULT_ELL, DEFAULT_SCALE);

        vector<FixArray> ir_tmp1(batch_size);
        uint64_t *tmp1 = new uint64_t[batch_size * d_module];
        uint64_t *tmp2 = new uint64_t[batch_size * d_module];
        for (size_t i = 0; i < batch_size; i++) {
            vec_x_gb[i].party = sci::PUBLIC;
            ir_tmp1[i] = fpmath_public->fix->sub(vec_x_gb[i], fix_mean_g[i].data[0]); // ?
            ir_tmp1[i] = fpmath->fix->mul(ir_tmp1[i], fix_ka.data[0], DEFAULT_ELL);   // ?
            ir_tmp1[i] = fpmath->fix->location_truncation(ir_tmp1[i], DEFAULT_SCALE); // ?
            for (size_t j = 0; j < d_module; j++) {
                tmp1[i * d_module + j] = ir_tmp1[i].data[j];
                tmp2[i * d_module + j] = delta_gb[i].data[j];
            }
        }
        conv->Ring_to_Prime(fix_div_ha.data[0], fix_div_ka.data[0], DEFAULT_ELL, party->parm->plain_mod);
        conv->Ring_to_Prime(tmp2, tmp2, batch_size * d_module, DEFAULT_ELL, party->parm->plain_mod);

        BFVLongCiphertext layernorm_secret_a(party->parm, fix_div_ha.data[0], party);
        BFVLongPlaintext tmp2_plain(party->parm, tmp2, batch_size * d_module);
        layernorm_secret_a.multiply_plain_inplace(tmp2_plain, party->parm->evaluator);
        auto ln_plain = layernorm_secret_a.decrypt(party);
        auto ln = ln_plain.decode(parm);
        std::cout << ln[0] << "\n";
        io->send_data(tmp1, batch_size * d_module * sizeof(uint64_t));
        BFVLongCiphertext::send(io, &layernorm_secret_a);

        return BFVLongCiphertext();
    } else {
        const double gb = dist(gen);
        uint64_t *prime_ha_xa = new uint64_t[batch_size * d_module];
        FixArray fix_ha_xa = fpmath->fix->input(sci::ALICE, batch_size * d_module, x, true, DEFAULT_ELL, DEFAULT_SCALE);
        BFVLongCiphertext ha_secret_a, attn_ha_secret_b;

        io->recv_data(prime_ha_xa, batch_size * d_module * sizeof(uint64_t));
        recv_mat(io, fix_ha_xa.data, fix_ha_xa.size);
        BFVLongCiphertext::recv(io, &ha_secret_a, parm->context);
        BFVLongCiphertext::recv(io, &attn_ha_secret_b, parm->context);

        BFVLongPlaintext attn_ha_plain = attn_ha_secret_b.decrypt(party);
        uint64_t *prime_xb = new uint64_t[batch_size * d_module];

        conv->Ring_to_Prime(input.data(), prime_xb, batch_size * d_module, DEFAULT_ELL, party->parm->plain_mod);

        BFVLongPlaintext xb_plain(party->parm, prime_xb, batch_size * d_module);
        BFVLongCiphertext xb_ha_secret_a = ha_secret_a.multiply_plain(xb_plain, party->parm->evaluator); // ha_xb
        xb_ha_secret_a.mod_switch_to_next_inplace(party->parm->evaluator);
        xb_ha_secret_a.add_plain_inplace(attn_ha_plain, party->parm->evaluator);
        BFVLongPlaintext ha_xa_plain(party->parm, prime_ha_xa, batch_size * d_module);
        xb_ha_secret_a.add_plain_inplace(ha_xa_plain, party->parm->evaluator);

        FixArray fix_gb =
            fpmath->fix->input(sci::BOB, batch_size * d_module,
                               sci::neg_mod(static_cast<int64_t>(gb * (1ULL << (DEFAULT_SCALE))), 1ULL << DEFAULT_ELL),
                               true, DEFAULT_ELL, DEFAULT_SCALE);
        uint64_t *prime_gb = new uint64_t[batch_size * d_module];
        conv->Ring_to_Prime(fix_gb.data, prime_gb, batch_size * d_module, DEFAULT_ELL, party->parm->plain_mod);
        BFVLongPlaintext gb_plain(party->parm, prime_gb, batch_size * d_module);

        xb_ha_secret_a.multiply_plain_inplace(gb_plain, party->parm->evaluator);
        xb_ha_secret_a.mod_switch_to_next_inplace(party->parm->evaluator);
        // Bob send [x_add*ha*gb]_a} to alice;
        BFVLongCiphertext::send(io, &xb_ha_secret_a);

        uint64_t *tmp1 = new uint64_t[batch_size * d_module];
        BFVLongCiphertext layernorm_secret_a;
        io->recv_data(tmp1, batch_size * d_module * sizeof(uint64_t));
        BFVLongCiphertext::recv(io, &layernorm_secret_a, party->parm->context);
        // tmp * gama
        uint64_t *gama_array = new uint64_t[batch_size * d_module];
        uint64_t *beta_array = new uint64_t[batch_size * d_module];
        for (size_t i = 0; i < batch_size; i++) {
            for (size_t j = 0; j < d_module; j++) {
                gama_array[i * d_module + j] = gamma[j];
                beta_array[i * d_module + j] =
                    sci::neg_mod(static_cast<int64_t>(beta[i]), static_cast<int64_t>(party->parm->plain_mod));
            }
        }
        FixArray gama_fix =
            fpmath->fix->input(sci::BOB, batch_size * d_module, gama_array, true, DEFAULT_ELL, DEFAULT_SCALE);
        FixArray gama_tmp1(sci::BOB, batch_size * d_module, true, DEFAULT_ELL, DEFAULT_SCALE);

        FixArray ret_tmp1 =
            fpmath->fix->input(sci::ALICE, batch_size * d_module, tmp1, true, DEFAULT_ELL, DEFAULT_SCALE);
        ret_tmp1.party = sci::PUBLIC;
        gama_tmp1 = fpmath->fix->mul(ret_tmp1, gama_fix, DEFAULT_ELL);
        gama_tmp1 = fpmath->fix->location_truncation(gama_tmp1, DEFAULT_SCALE);
        conv->Ring_to_Prime(tmp1, tmp1, batch_size * d_module, DEFAULT_ELL, party->parm->plain_mod);

        BFVLongPlaintext tmp1_plain(party->parm, tmp1, batch_size * d_module);
        layernorm_secret_a.multiply_plain_inplace(tmp1_plain, party->parm->evaluator);
        layernorm_secret_a.mod_switch_to_next_inplace(party->parm->evaluator);
        BFVLongPlaintext beta_plain(party->parm, beta_array, batch_size * d_module);
        layernorm_secret_a.add_plain_inplace(beta_plain, party->parm->evaluator);
        return ha_secret_a;
    }
    delete[] x;
}
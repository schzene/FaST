#include "SCI/utils.h"
#include <model.h>
#define TEST

bfv_matrix fixed_mean(bfv_matrix input, size_t row, size_t column, uint64_t mask) {
    bfv_matrix result(row);
    size_t i, j;
    for (i = 0; i < row; i++) {
        for (j = 0; j < column; j++) {
            result[i] += input[i * column + j];
        }
        result[i] /= column;
        result[i] &= mask;
    }
    return result;
}

bfv_matrix fixed_standard_deviation(bfv_matrix input, bfv_matrix means, size_t row, size_t column, uint64_t mask, FixOp *fixop) {
    bfv_matrix result(row);
    size_t i, j;
    for (i = 0; i < row; i++) {
        for (j = 0; j < column; j++) {
            result[i] += (input[i * column + j] - means[i]) * (input[i * column + j] - means[i]);
        }
        result[i] /= column;
        result[i] = (uint64_t)sqrt(result[i]) & mask;
    }
    return result;
}

class SecureLayerNorm1 {
    BFVParm *bfv_parm;
    BFVKey *alice;
    BFVKey *bob;

    sci::IOPack *iopack;
    sci::OTPack *otpack;

    FixOp *fix_alice;
    FixOp *fix_bob;
    FixOp *fix_public;

public:
    SecureLayerNorm1(BFVParm *bfv_parm_, BFVKey *alice_, BFVKey *bob_,
                     sci::IOPack *iopack_, sci::OTPack *otpack_) : bfv_parm(bfv_parm_),
                                                                   alice(alice_),
                                                                   bob(bob_),
                                                                   iopack(iopack_),
                                                                   otpack(otpack_) {
        fix_alice = new FixOp(sci::ALICE, iopack, otpack);
        fix_bob = new FixOp(sci::BOB, iopack, otpack);
        fix_public = new FixOp(sci::PUBLIC, iopack, otpack);
    }

    ~SecureLayerNorm1() {
        delete fix_alice;
        delete fix_bob;
        delete fix_public;
    }

    void forward(BFVLongCiphertext &attn_s, const bfv_matrix &input_a, const bfv_matrix &input_b) {

        sci::PRG128 prg;
        size_t i, j;

        uint64_t *intput_data_a = new uint64_t[input_a.size()];
        uint64_t *intput_data_b = new uint64_t[input_b.size()];

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(-1, 1);

        double ha1 = dist(gen), ha2 = dist(gen);
        double ha2_div_ha1 = ha2 / ha1;

        uint64_t fix_ha1 = sci::neg_mod(static_cast<int64_t>(ha1 * (1ULL << DEFAULT_BITWIDTH)), DEFAULT_ELL);
        uint64_t fix_ha2 = sci::neg_mod(static_cast<int64_t>(ha2 * (1ULL << DEFAULT_BITWIDTH)), DEFAULT_ELL);
        uint64_t fix_h2_div_h1 = sci::neg_mod(static_cast<int64_t>(ha2_div_ha1 * (1ULL << DEFAULT_BITWIDTH)), DEFAULT_ELL);

        for (size_t i = 0; i < input_a.size(); i++) {
            intput_data_a[i] = input_a[i];
            intput_data_b[i] = input_b[i];
        }

        FixArray fix_input_a = fix_alice->input(sci::ALICE, input_a.size(), intput_data_a);
        FixArray fix_input_b = fix_bob->input(sci::BOB, input_b.size(), intput_data_b);
        FixArray ha1_xa = fix_public->mul(fix_input_a, fix_ha1); // TODO:: Fixarry * Fixarry; Maby this is correct!
        BFVLongCiphertext ha2_div_ha1_secret_a(bfv_parm, ha2_div_ha1, alice);
        // TODO:: ha1_xa--ss_to_he  ell to plain_mod

        BFVLongPlaintext ha2_plain(bfv_parm, fix_ha2);
        BFVLongCiphertext ha2_secret_a(ha2_plain, alice);
        BFVLongCiphertext attn_ha2_b = attn_s.multiply_plain(ha2_plain, bfv_parm->evaluator);

        // send H1 = {hc1_xc, hc2_div_hc1_secret_a, hc2_secret_a, attn_hc2_s} to bob

        // bob receive H1, and get hc1_xc, hc2_div_hc1_secret_a, hc2_secret_a, attn_hc2
        auto attn_ha2_plain = attn_ha2_b.decrypt(bob);

        // TODO:: input_b--ss_to_he ell to plain_mod
        BFVLongPlaintext input_b_plain(bfv_parm, input_b);
        BFVLongCiphertext xha1_secret_a = ha2_secret_a.multiply_plain(input_b_plain, bfv_parm->evaluator);
        // attn_ha2_plain.mod_switch_to_inplace(xha1_secret_a.parms_id(), evaluator); // error:  plain is not valid for encryption parameters ! just need to neg_mod to plain_mod from ell;
        xha1_secret_a.add_plain_inplace(attn_ha2_plain, bfv_parm->evaluator);

        // ------------------------------------------------------------------------------
        // something is wrong here.
        BFVLongPlaintext ha1_xa_plain(bfv_parm, ha1_xa.data, ha1_xa.size); // error:  fixrry need to mod to plain_mod
        ha2_div_ha1_secret_a.multiply_plain_inplace(ha1_xa_plain, bfv_parm->evaluator);
        // ha2_div_ha1_secret_a.mod_switch_to_inplace(xha1_secret_a.parms_id(), evaluator);
        xha1_secret_a.add_inplace(ha2_div_ha1_secret_a, bfv_parm->evaluator);

#ifdef TEST
        int NL_ELL = 29;
        uint64_t mask = (NL_ELL == 64 ? -1 : ((1ULL << NL_ELL) - 1));
        auto attn_plain = attn_s.decrypt(bob);
        auto attn = attn_plain.decode(bfv_parm);
        for (size_t i = 0; i < batch_size * d_module; i++) {
            attn[i] = (attn[i] + input_a[i] + input_b[i]) & mask;
        }
        auto mu = fixed_mean(attn, batch_size, d_module, mask);
        auto sigma = fixed_standard_deviation(attn, mu, batch_size, d_module, mask, fix_public);
        for (size_t i = 0; i < batch_size; i++) {
            for (size_t j = 0; j < d_module; j++) {
                attn[i * d_module + j] = (attn[i * d_module + j] - mu[i]) & mask;
                // attn[i * d_module + j] = (attn[i * d_module + j] / sigma[i]) & mask;
                // attn[i * d_module + j] *= gamma[i * d_module + j];
                // attn[i * d_module + j] += beta[i * d_module + j];
            }
        }
        std::cout << attn[0] << " " << sigma[0] << "\n";
#endif
        delete intput_data_b;
        delete intput_data_a;
    }
};

int main() {
    int NL_ELL = 29;
    uint64_t mask = (NL_ELL == 64 ? -1 : ((1ULL << NL_ELL) - 1));

    BFVParm *bfv_parm = new BFVParm(8192, {54, 54, 55, 55}, default_prime_mod.at(29));

    BFVKey *alice = new BFVKey(sci::ALICE, bfv_parm);
    BFVKey *bob = new BFVKey(sci::BOB, bfv_parm);

    bfv_matrix attn(batch_size * d_module),
        input_a(batch_size * d_module), input_b(batch_size * d_module);

    random_bfv_mat(attn);
    random_bfv_mat(input_a);
    random_bfv_mat(input_b);
    for (size_t i = 0; i < batch_size * d_module; i++) {
        attn[i] &= mask;
        input_a[i] &= mask;
        input_b[i] &= mask;
    }

    BFVLongPlaintext attn_plain(bfv_parm, attn);
    BFVLongCiphertext attn_secret_s(attn_plain, bob);
    size_t i, j;
    sci::IOPack *iopack;
    sci::OTPack *otpack;
    SecureLayerNorm1 *sec_ln1 = new SecureLayerNorm1(bfv_parm, alice, bob, iopack, otpack);
    sec_ln1->forward(attn_secret_s, input_a, input_b);
    int length = 10;
    uint64_t *share = new uint64_t[length];

    sci::PRG128 prg_con;
    prg_con.random_mod_p<uint64_t>(share, length, bfv_parm->plain_mod);
    BFVLongCiphertext ct;
    // ss_to_he(bfv_parm, alice, share, ct, length, int 37);
    delete bfv_parm;
    delete alice;
    delete bob;
    delete iopack;
    delete otpack;
    delete sec_ln1;
    delete share;
}
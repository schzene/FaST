#include "SCI/utils.h"
#include <model.h>
#define TEST

class SecureLayerNorm1 {
    BFVparm *bfv_alice_parm;
    BFVKey *alice;
    BFVKey *bob;
    BatchEncoder *encoder;
    Evaluator *evaluator;

    sci::IOPack *iopack;
    sci::OTPack *otpack;

public:
    SecureLayerNorm1(BFVparm *bfv_alice_parm_, BFVKey *alice_, BFVKey *bob_,
                     BatchEncoder *encoder_, Evaluator *evaluator_, sci::IOPack *iopack_, sci::OTPack *otpack_) : bfv_alice_parm(bfv_alice_parm_),
                                                                                                                  alice(alice_),
                                                                                                                  bob(bob_),
                                                                                                                  encoder(encoder_),
                                                                                                                  evaluator(evaluator_),
                                                                                                                  iopack(iopack_),
                                                                                                                  otpack(otpack_) {}
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

        uint64_t fix_ha1 = sci::neg_mod(static_cast<int64_t>(ha1 * (1ULL << 13)), 64);
        uint64_t fix_ha2 = sci::neg_mod(static_cast<int64_t>(ha2 * (1ULL << 13)), 64);
        uint64_t fix_h2_div_h1 = sci::neg_mod(static_cast<int64_t>(ha2_div_ha1 * (1ULL << 13)), 8192);

        for (size_t i = 0; i < input_a.size(); i++) {
            intput_data_a[i] = input_a[i];
            intput_data_b[i] = input_b[i];
        }

        FixOp *fix_alice = new FixOp(sci::ALICE, iopack, otpack);
        FixOp *fix_bob = new FixOp(sci::BOB, iopack, otpack);
        FixOp *fix_public = new FixOp(sci::PUBLIC, iopack, otpack);
        FixArray fix_input_a = fix_alice->input(sci::ALICE, input_a.size(), intput_data_a, true, 64, 13);
        FixArray fix_input_b = fix_bob->input(sci::BOB, input_b.size(), intput_data_b, true, 64, 13);
        FixArray ha1_xa = fix_public->mul(fix_input_a, fix_ha1); // TODO:: Fixarry * Fixarry; Maby this is correct!
        std::cout << "the scale of data: " << ha1_xa.s << "\n";
        BFVLongCiphertext ha2_div_ha1_secret_a(ha2_div_ha1, alice, encoder);
        // TODO:: ha1_xa--ss_to_he  ell to plain_mod

        BFVLongPlaintext ha2_plain(fix_ha2, encoder);
        BFVLongCiphertext ha2_secret_a(ha2_plain, alice);
        BFVLongCiphertext attn_ha2_b = attn_s.multiply_plain(ha2_plain, evaluator);

        // send H1 = {hc1_xc, hc2_div_hc1_secret_a, hc2_secret_a, attn_hc2_s} to bob

        // bob receive H1, and get hc1_xc, hc2_div_hc1_secret_a, hc2_secret_a, attn_hc2
        auto attn_ha2_plain = attn_ha2_b.decrypt(bob);

        // TODO:: input_b--ss_to_he ell to plain_mod
        BFVLongPlaintext input_b_plain(bfv_alice_parm, input_b, encoder);
        BFVLongCiphertext xha1_secret_a = ha2_secret_a.multiply_plain(input_b_plain, evaluator);
        // attn_ha2_plain.mod_switch_to_inplace(xha1_secret_a.parms_id(), evaluator); // error:  plain is not valid for encryption parameters ! just need to neg_mod to plain_mod from ell;
        xha1_secret_a.add_plain_inplace(attn_ha2_plain, evaluator);

        // ------------------------------------------------------------------------------
        // something is wrong here.
        BFVLongPlaintext ha1_xc_plain(bfv_alice_parm, ha1_xa, encoder); // error:  fixrry need to mod to plain_mod
        ha2_div_ha1_secret_a.multiply_plain_inplace(ha1_xc_plain, evaluator);
        // ha2_div_ha1_secret_a.mod_switch_to_inplace(xha1_secret_a.parms_id(), evaluator);
        xha1_secret_a.add_inplace(ha2_div_ha1_secret_a, evaluator);
    }
};

int main() {
    std::cout << default_prime_mod.at(29) << "\n";
    BFVparm *bfv_alice_parm = new BFVparm(sci::ALICE, 8192, {54, 54, 55, 55}, default_prime_mod.at(29));
    BFVparm *bfv_bob_parm = new BFVparm(sci::BOB, 8192, {54, 54, 55, 55}, default_prime_mod.at(29));

    BFVKey *alice = new BFVKey(bfv_alice_parm->party, bfv_alice_parm->context);
    BFVKey *bob = new BFVKey(bfv_bob_parm->party, bfv_bob_parm->context);

    BatchEncoder *encoder = new BatchEncoder(*bfv_bob_parm->context);

    Evaluator *evaluator = new Evaluator(*bfv_bob_parm->context);
    bfv_matrix attn(batch_size * d_module),
        input_a(batch_size * d_module), input_b(batch_size * d_module);

    random_bfv_mat(attn);
    random_bfv_mat(input_a);
    random_bfv_mat(input_b);

    BFVLongPlaintext attn_plain(bfv_bob_parm, attn, encoder);
    BFVLongCiphertext attn_secret_s(attn_plain, bob);
    size_t i, j;
    sci::IOPack *iopack;
    sci::OTPack *otpack;
    SecureLayerNorm1 *sec_ln1 = new SecureLayerNorm1(bfv_alice_parm, alice, bob, encoder, evaluator, iopack, otpack);
    // sec_ln1->forward(attn_secret_s, input_a, input_b);
    int length = 10;
    uint64_t *share = new uint64_t[length];

    sci::PRG128 prg_con;
    prg_con.random_mod_p<uint64_t>(share, length, (1ULL << 13));
    BFVLongCiphertext ct;
    // ss_to_he(bfv_bob_parm, alice, share, ct, length, int 37);
}
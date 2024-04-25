#include <module.h>

class SecureLayerNorm1 {
    CKKSKey *alice;
    CKKSKey *bob;
    CKKSEncoder *encoder;
    Evaluator *evaluator;

public:
    SecureLayerNorm1(CKKSKey *alice_, CKKSKey *bob_,
                     CKKSEncoder *encoder_, Evaluator *evaluator_) : alice(alice_),
                                                                     bob(bob_),
                                                                     encoder(encoder_),
                                                                     evaluator(evaluator_) {}

    void forward(LongCiphertext &attn_s, const std::vector<double> &input) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(-1, 1);
        size_t i, j;
        std::vector<double> input_a(batch_size * d_module), input_b(batch_size * d_module);
        random_mat(input_b);
        for (i = 0; i < batch_size * d_module; i++) {
            input_a[i] = input[i] - input_b[i];
        }

        double hc1 = dist(gen), hc2 = dist(gen);
        std::vector<double> hc1_xc(input.size());
        for (i = 0; i < batch_size * d_module; i++) {
            hc1_xc[i] = hc1 * input_a[i];
        }
        LongCiphertext hc2_div_hc1_secret_a(hc2 / hc1, alice, encoder);
        LongPlaintext hc2_plain(hc2, encoder);
        LongCiphertext hc2_secret_a(hc2_plain, alice);
        LongCiphertext attn_hc2_s = attn_s.multiply_plain(hc2_plain, evaluator);
        // send H1 = {hc1_xc, hc2_div_hc1_secret_a, hc2_secret_a, attn_hc2_s} to bob

        // Bob receive H1, and get hc1_xc, hc2_div_hc1_secret_a, hc2_secret_a, attn_hc2
        auto attn_hc2_plain = attn_hc2_s.decrypt(bob);
        LongPlaintext input_b_plain(input_b, encoder);
        LongCiphertext xhc1_secret_a = hc2_secret_a.multiply_plain(input_b_plain, evaluator);
        attn_hc2_plain.mod_switch_to_inplace(xhc1_secret_a.parms_id(), evaluator);
        xhc1_secret_a.add_plain_inplace(attn_hc2_plain, evaluator);

        LongPlaintext hc1_xc_plain(hc1_xc, encoder);
        hc2_div_hc1_secret_a.multiply_plain_inplace(hc1_xc_plain, evaluator);
        hc2_div_hc1_secret_a.mod_switch_to_inplace(xhc1_secret_a.parms_id(), evaluator);
        xhc1_secret_a.add_inplace(hc2_div_hc1_secret_a, evaluator);
        auto x_ = xhc1_secret_a.decrypt(alice);
        auto x = x_.decode(encoder);
        print_mat(x, batch_size, d_module);

        double gs = dist(gen);
        gs = 1;
        LongPlaintext gs_plain(gs, encoder);
        gs_plain.mod_switch_to_inplace(xhc1_secret_a.parms_id(), evaluator);
        xhc1_secret_a.multiply_plain_inplace(gs_plain, evaluator);
        // send H2 = {xhc1_secret_a} to alice;

        // alice receive H2, and get x * gs
        auto xgs_plain = xhc1_secret_a.decrypt(alice);
        auto xgs = xgs_plain.decode(encoder);
        for (i = 0; i < batch_size * d_module; i++) {
            xgs[i] /= hc1;
        }
        // print_mat(xgs, batch_size, d_module);
        double kc1 = dist(gen), kc2 = dist(gen);
        LongCiphertext kc1_secret_a(kc1, alice, encoder);
        LongCiphertext kc2_div_kc1_secret_a(kc2 / kc1, alice, encoder);
    }
};

int main() {
    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    SEALContext *context = new SEALContext(parms);
    CKKSEncoder *encoder = new CKKSEncoder(*context);
    Evaluator *evaluator = new Evaluator(*context);
    CKKSKey *alice = new CKKSKey(1, context);
    CKKSKey *bob = new CKKSKey(2, context);

    std::vector<double> attn(batch_size * d_module), input(batch_size * d_module);
    random_mat(attn);
    random_mat(input);
    LongPlaintext attn_plain(attn, encoder);
    LongCiphertext attn_secret_s(attn_plain, bob);

    SecureLayerNorm1 *sec_ln1 = new SecureLayerNorm1(alice, bob, encoder, evaluator);
    sec_ln1->forward(attn_secret_s, input);
    for (size_t i = 0; i < batch_size * d_module; i++) {
        // attn[i] += input[i];
    }
    print_mat(attn, batch_size, d_module);

    delete sec_ln1;
    delete context;
    delete encoder;
    delete evaluator;
    delete alice;
    delete bob;
}
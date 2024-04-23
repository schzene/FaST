#include <protocols/attention.h>

int main(int argc, const char **argv) {
    size_t poly_modulus_degree = 8192;
    size_t slot_count = poly_modulus_degree / 2;
    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    SEALContext *context = new SEALContext(parms);
    CKKSEncoder *encoder = new CKKSEncoder(*context);
    Evaluator *evaluator = new Evaluator(*context);
    int party_ = argc > 1 ? 2 : 1;
    if (party_ == ALICE) {
        std::cout << "Party: ALICE" << "\n";
    } else if (party_ == BOB) {
        std::cout << "Party: BOB" << "\n";
    }
    CKKSKey *party = new CKKSKey(party_, context, slot_count);

    IOPack *io_pack = new IOPack(party_);
    size_t inp_seq = 16, d_module = 32, d_k = 28;
    std::vector<double> input(inp_seq * d_module);
    random_mat(input);
    Attention *attn = new Attention(party, context, io_pack, input, d_module, d_k);
    attn->forward();
    delete io_pack;
    delete party;
    delete evaluator;
    delete encoder;
    delete context;
    return 0;
}
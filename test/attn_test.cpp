#include <protocols.h>

int main(int argc, const char **argv) {
    size_t poly_modulus_degree = 8192;
    size_t slot_count = poly_modulus_degree / 2;
    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    SEALContext *context = new SEALContext(parms);
    CKKSEncoder *encoder = new CKKSEncoder(*context);
    Evaluator *evaluator = new Evaluator(*context);
    int party_ = argc > 1 ? 1 : 2;
    if (party_ == ALICE) {
        std::cout << "Party: ALICE"
                  << "\n";
    } else if (party_ == BOB) {
        std::cout << "Party: BOB"
                  << "\n";
    }
    CKKSKey *party = new CKKSKey(party_, context, slot_count);

    IOPack *io_pack = new IOPack(party_);
    size_t inp_seq = 128, d_module = 768, d_k = 768, n_head = 1;
    printf("input seq:        %ld\nd_module:         %ld\nd_k:              %ld\nnumber of heads:  %ld\n", inp_seq, d_module, d_k, n_head);
    std::vector<double> input(inp_seq * d_module);
    random_mat(input);
    // Attention *attn = new Attention(party, context, io_pack, input, d_module, d_k, 0);
    Multi_Head_Attention *attn = new Multi_Head_Attention(party, context, io_pack, n_head, d_module, d_k);

    LongCiphertext result;
    INIT_TIMER;
    START_TIMER;
    result = attn->forward(input);
    STOP_TIMER("attention");
    auto comm = io_pack->get_comm();
    if (comm < 1024) {
        printf("data size of communication: %ld B\n", comm);
    } else if (comm < 1024 * 1024) {
        printf("data size of communication: %.2lf KB\n", comm / 1024.);
    } else if (comm < 1024 * 1024 * 1024) {
        printf("data size of communication: %.2lf MB\n", comm / (1024. * 1024.));
    } else {
        printf("data size of communication: %.2lf MB\n", comm / (1024. * 1024. * 1024.));
    }
    std::cout << "rounds of communication: " << io_pack->get_rounds() << "\n";

    delete attn;
    delete io_pack;
    delete party;
    delete evaluator;
    delete encoder;
    delete context;
    return 0;
}
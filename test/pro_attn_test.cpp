#include <protocols.h>

int main(int argc, const char **argv) {
    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    SEALContext *context = new SEALContext(parms);
    CKKSEncoder *encoder = new CKKSEncoder(*context);
    Evaluator *evaluator = new Evaluator(*context);
    int party_ = argc > 1 ? 2: 1;
    if (party_ == ALICE) {
        std::cout << "Party: ALICE"
                  << "\n";
    } else if (party_ == BOB) {
        std::cout << "Party: BOB"
                  << "\n";
    }
    CKKSKey *party = new CKKSKey(party_, context);

    IOPack *io_pack = new IOPack(party_);
    printf("batch size:       %d\nd_module:         %d\nnumber of heads:  %d\n", batch_size, d_module, n_heads);
    std::vector<double> input(batch_size * d_module);
    random_mat(input, 0, 0.01);
    // Attention *attn = new Attention(party, context, io_pack, input, d_module, d_k, 0);
    Multi_Head_Attention *attn = new Multi_Head_Attention(party, encoder, evaluator, io_pack);

    LongCiphertext result;
    INIT_TIMER;
    START_TIMER;
    attn->forward(input);
    STOP_TIMER("Multi-Head Attention");
    size_t comm = io_pack->get_comm();
    size_t rounds = io_pack->get_rounds() / n_heads;
    if (comm < 1024) {
        printf("data size of communication: %ld B\n", comm);
    } else if (comm < 1024 * 1024) {
        printf("data size of communication: %.2lf KB\n", comm / 1024.);
    } else if (comm < 1024 * 1024 * 1024) {
        printf("data size of communication: %.2lf MB\n", comm / (1024. * 1024.));
    } else {
        printf("data size of communication: %.2lf MB\n", comm / (1024. * 1024. * 1024.));
    }
    std::cout << "rounds of communication: " << rounds << "\n";

    delete attn;
    delete io_pack;
    delete party;
    delete evaluator;
    delete encoder;
    delete context;
    return 0;
}
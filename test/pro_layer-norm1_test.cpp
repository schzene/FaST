#include <protocols.h>

int main(int argc, const char **argv) {
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
    CKKSKey *party = new CKKSKey(party_, context);

    IOPack *io_pack = new IOPack(party_);
    printf("batch size:       %d\nd_module:         %d\n", batch_size, d_module);
    matrix input(batch_size * d_module);
    random_mat(input);
    LayerNorm1 *ln1 = new LayerNorm1(party, encoder, evaluator, io_pack);

    LongCiphertext attn_secret_b;
    if (party_ == ALICE) {
        LongCiphertext::recv(io_pack, &attn_secret_b, context);
    } else if (party_ == BOB) {
        matrix attn(batch_size * d_module);
        random_mat(attn);
        LongPlaintext attn_plain(attn, encoder);
        LongCiphertext attn_s_b(attn_plain, party);
        LongCiphertext::send(io_pack, &attn_s_b);
    }
    io_pack->io->num_rounds = 0;
    io_pack->io_rev->num_rounds = 0;
    INIT_TIMER;
    START_TIMER;
    LongCiphertext result = ln1->forward(attn_secret_b, input);
    STOP_TIMER("LayerNorm1");
    size_t comm = io_pack->get_comm();
    size_t rounds = io_pack->get_rounds();
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

    delete ln1;
    delete io_pack;
    delete party;
    delete evaluator;
    delete encoder;
    delete context;
    return 0;
}
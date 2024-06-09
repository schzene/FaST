#include <protocols.h>

int main(int argc, const char **argv) {
    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    SEALContext *context = new SEALContext(parms);
    CKKSEncoder *encoder = new CKKSEncoder(*context);
    Evaluator *evaluator = new Evaluator(*context);
    int party_ = argc > 1 ? 1 : 2;
    if (party_ == sci::ALICE) {
        std::cout << "Party: ALICE"
                  << "\n";
    } else if (party_ == sci::BOB) {
        std::cout << "Party: BOB"
                  << "\n";
    }
    CKKSKey *party = new CKKSKey(party_, context);

    sci::IOPack *io_pack = new sci::IOPack(party_, 32000);
    sci::NetIO *io = io_pack->io;
    sci::NetIO *io_rev = io_pack->io_rev;

    printf("batch size:       %d\nd_module:         %d\n", batch_size, d_module);
    matrix input(batch_size * d_module);
    random_mat(input);
    LayerNorm *ln = new LayerNorm(party, encoder, evaluator, io);

    io->num_rounds = 0;
    io_rev->num_rounds = 0;
    LongCiphertext attn_secret_b;
    if (party_ == sci::ALICE) {
        LongCiphertext::recv(io, &attn_secret_b, context);
    } else if (party_ == sci::BOB) {
        matrix attn(batch_size * d_module);
        random_mat(attn);
        LongPlaintext attn_plain(attn, encoder);
        LongCiphertext attn_s_b(attn_plain, party);
        LongCiphertext::send(io, &attn_s_b);
    }

    INIT_TIMER;
    START_TIMER;
    LongCiphertext result = ln->forward(attn_secret_b, input);
    STOP_TIMER("LayerNorm");
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

    delete ln;
    delete io_pack;
    delete party;
    delete evaluator;
    delete encoder;
    delete context;
    return 0;
}
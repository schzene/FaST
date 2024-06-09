#include <protocols.h>

// int party_, port = 32000;
// int num_threads = 12;
// string address = "127.0.0.1";

// int dim = 128 * 3072;

int main(int argc, const char **argv) {

    /************* Argument Parsing  ************/
    /********************************************/
    // ArgMapping amap;
    // amap.arg("r", party_, "Role of party: ALICE = 1; BOB = 2");
    // amap.arg("p", port, "Port Number");
    // amap.arg("N", dim, "Number of operation operations");
    // amap.arg("nt", num_threads, "Number of threads");
    // amap.arg("ip", address, "IP Address of server (ALICE)");

    // amap.parse(argc, argv);

    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    SEALContext *context = new SEALContext(parms);
    CKKSEncoder *encoder = new CKKSEncoder(*context);
    Evaluator *evaluator = new Evaluator(*context);
    int party_ = argc > 1 ? 2 : 1;
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
    printf("batch size:       %d\nd_module:         %d\nFFN_dim:          %d\n", batch_size, d_module, ffn_dim);
    matrix input(batch_size * d_module);
    random_mat(input, 0, 0.01);
    FFN *ffn = new FFN(party, encoder, evaluator, io);

    LongCiphertext ln1_secret_a;
    if (party_ == sci::ALICE) {
        matrix ln1(batch_size * d_module);
        random_mat(ln1);
        LongPlaintext ln1_plain(ln1, encoder);
        LongCiphertext ln1_s_a(ln1_plain, party);
        // io_pack->io_rev->send_data
        LongCiphertext::send(io, &ln1_s_a);
    } else if (party_ == sci::BOB) {
        LongCiphertext::recv(io, &ln1_secret_a, context);
    }

    io->num_rounds = 0;
    io_rev->num_rounds = 0;
    // INIT_TIMER;
    // START_TIMER;
    ffn->forward(ln1_secret_a);
    // STOP_TIMER("test: Feed Forward");
    size_t comm = io_pack->get_comm();
    size_t rounds = io_pack->get_rounds(); // / n_heads;
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

    delete ffn;
    delete io_pack;
    delete party;
    delete evaluator;
    delete encoder;
    delete context;
    return 0;
}
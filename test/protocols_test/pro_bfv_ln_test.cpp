#include <protocols.h>

int main(int argc, const char **argv) {
    if (argc > 1) {
        int party_ = argv[1][0] - '0';
        assert(party_ == sci::ALICE || party_ == sci::BOB);
        if (party_ == sci::ALICE) {
            std::cout << "Party: ALICE"
                      << "\n";
        } else if (party_ == sci::BOB) {
            std::cout << "Party: BOB"
                      << "\n";
        }
        BFVParm *bfv_parm = new BFVParm(8192, {54, 54, 55, 55}, default_prime_mod.at(29));
        BFVKey *party = new BFVKey(party_, bfv_parm);
        sci::IOPack *iopack = new sci::IOPack(party_, 56789);
        sci::OTPack *otpack = new sci::OTPack(iopack, party_);
        sci::NetIO *io = iopack->io;
        Conversion *conv = new Conversion();
        FPMath *fpmath = new FPMath(party_, iopack, otpack);
        FPMath *fpmath_public = new FPMath(sci::PUBLIC, iopack, otpack);

        FixOp *fix_party = new FixOp(party_, iopack, otpack);
        FixOp *fix_public = new FixOp(sci::PUBLIC, iopack, otpack);
        bfv_matrix input(batch_size * d_module);
        random_bfv_mat(input);

        FixedLayerNorm *ln = new FixedLayerNorm(party, bfv_parm, io, fpmath, fpmath_public, conv, 0, false);
        BFVLongCiphertext attn_secret_b;
        if (party_ == sci::ALICE) {
            BFVLongCiphertext::recv(iopack->io, &attn_secret_b, bfv_parm->context);
        } else if (party_ == sci::BOB) {
            bfv_matrix attn(batch_size * d_module);
            random_bfv_mat(attn);
            BFVLongPlaintext attn_plain(bfv_parm, attn);
            BFVLongCiphertext attn_s_b(attn_plain, party);
            BFVLongCiphertext::send(iopack->io, &attn_s_b);
        }
        printf("batch size:       %d\nd_module:         %d\n", batch_size, d_module);
        INIT_TIMER;
        START_TIMER;
        BFVLongCiphertext result = ln->forward(attn_secret_b, input);
        STOP_TIMER("LayerNorm");
        size_t comm = iopack->get_comm();
        size_t rounds = iopack->get_rounds();
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
        delete fix_public;
        delete fix_party;
        delete otpack;
        delete iopack;
        delete party;
        delete bfv_parm;
    } else {
        std::cout << "No party input\n";
    }
    return 0;
}
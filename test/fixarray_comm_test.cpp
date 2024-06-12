#include <SCI/utils.h>
#include <utils.h>

int main(int argc, const char **argv) {
    if (argc > 1) {
        int party_ = argv[1][0] - '0';
        bfv_matrix test(10000);
        random_bfv_mat(test);
        assert(party_ == sci::ALICE || party_ == sci::BOB);
        if (party_ == sci::ALICE) {
            std::cout << "Party: ALICE"
                      << "\n";
            
        } else if (party_ == sci::BOB) {
            std::cout << "Party: BOB"
                      << "\n";
        }
        sci::IOPack *iopack = new sci::IOPack(party_, 56789);
        sci::OTPack *otpack = new sci::OTPack(iopack, party_);
        FixOp *fix_party = new FixOp(party_, iopack, otpack);
        if (party_ == sci::ALICE) {
            FixArray fix_test = fix_party->input(sci::PUBLIC, test.size(), test.data());
            fix_party->send_fix_array(fix_test);
        } else if (party_ == sci::BOB) {
            FixArray fix_test;
            std::cout <<fix_test.ell << "\n";
            fix_party->recv_fix_array(fix_test);
        }

        delete fix_party;
        delete otpack;
        delete iopack;
    }
}
#include "SCI/utils.h"
#include <model.h>
#define TEST

int main() {
    int NL_ELL = 37;
    uint64_t mask_x = (NL_ELL == 64 ? -1 : ((1ULL << NL_ELL) - 1));
    // uint64_t ell_mask = (1ULL << 37) - 1; // 2 ** 37 -1

    int length = 2 * 50;
    uint64_t *random_share = new uint64_t[length];
    sci::PRG128 prg;
    prg.random_data(random_share, length * sizeof(uint64_t));
    // for (int i = 0; i < 10; i++) {
    //     std::cout << random_share[i] << " ";
    // }

    // std::cout << "\n";
    std::cout << "mask_x:" << mask_x << " \n";
    for (int i = 0; i < length; i++) {
        random_share[i] &= mask_x;
    }
    for (int i = 0; i < 10; i++) {
        std::cout << random_share[i] << " ";
    }
    std::cout << "\n";

    BFVParm *bfv_parm = new BFVParm(8192, {54, 54, 55, 55}, default_prime_mod.at(29));

    BFVKey *alice = new BFVKey(sci::ALICE, bfv_parm);

    sci::OTPack *otpack;
    sci::IOPack *iopack;
    MillionaireProtocol *mill;

    sci::PRG128 prg2;
    size_t size = 5;
    uint64_t *x = new uint64_t[size];
    prg2.random_data(x, size * sizeof(uint64_t));
    for (int i = 0; i < 10; i++) {
        std::cout << "no signed:" << x[i] << " ";
    }
    std::cout << "\n";
    FixOp *fix = new FixOp(sci::PUBLIC, iopack, otpack);
    FixArray input = fix->input(sci::PUBLIC, size, random_share, true, 64, 13);

    print_fix(input);

    delete[] random_share;
    delete bfv_parm;
    delete alice;
    delete otpack;
    delete iopack;
    delete mill;
    delete[] x;
    delete fix;
}

// const size_t bfv_poly_modulus_degree = 8192;
// const size_t bfv_slot_count = bfv_poly_modulus_degree;
// const vector<int> bfv_coeff_bit_sizes = {54, 54, 55, 55};
// const int32_t bitlength = 29;
// const uint64_t bfv_plain_mod = default_prime_mod.at(bitlength);
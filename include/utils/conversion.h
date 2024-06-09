#ifndef FAST_HE_BFV_CONVERSION_H__
#define FAST_HE_BFV_CONVERSION_H__

#include "SCI/utils.h"
#include "he-bfv.h"
#include "vector"
class Conversion {
public:
    int party;
    BFVparm *bfvparm;
    BFVKey *bfvkey;
    BatchEncoder *encoder;
    Conversion(BFVparm *bfvparm);

    ~Conversion();

    void he_to_ss(BFVparm *bfvparm, BFVLongCiphertext *ciphertext, uint64_t *share, bool ring);

    void he_to_ss(BFVparm *bfvparm, BFVLongCiphertext *ciphertext, bfv_matrix share, bool ring);

    void ss_to_he(BFVparm *bfvparm, uint64_t *share, BFVLongCiphertext *ciphertext, int length, int bw);

    void ss_to_he(BFVparm *bfvparm, BFVKey *bfvkey, uint64_t *share, BFVLongCiphertext *ciphertext, int length, int bw);
};

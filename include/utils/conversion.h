#ifndef FAST_HE_BFV_CONVERSION_H__
#define FAST_HE_BFV_CONVERSION_H__

#include "SCI/utils.h"
#include "he-bfv.h"
#include <vector>

class Conversion {
public:
    BFVKey *party;
    BFVparm *bfvparm;
    BatchEncoder *encoder;
    Conversion(BFVKey *_party, BFVparm *_bfvparm, BatchEncoder *_encoder) : party(_party), bfvparm(_bfvparm), encoder(_encoder) {}

    ~Conversion() {}

    void he_to_ss(BFVLongCiphertext &ciphertext, uint64_t *share, Evaluator *evaluator);

    inline void he_to_ss(BFVLongCiphertext &ciphertext, bfv_matrix share, Evaluator *evaluator) {
        he_to_ss(ciphertext, share.data(), evaluator);
    }

    void ss_to_he(uint64_t *share, BFVLongCiphertext &ciphertext, int length, int bw);

    void ss_to_he(BFVKey *bfvkey, uint64_t *share, BFVLongCiphertext &ciphertext, int length, int bw);
};

#endif
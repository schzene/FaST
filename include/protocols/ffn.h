#ifndef FAST_FFN_H__
#define FAST_FFN_H__
#include "protocol.h"

class FFN : public Protocol {
    LongCiphertext f3(const LongCiphertext &x1, const LongCiphertext &x2, const LongCiphertext &x3);
    LongCiphertext f2(const LongCiphertext &x1, const LongCiphertext &x2, const LongCiphertext &x4);
    LongCiphertext f1(const LongCiphertext &x1, const LongCiphertext &x2, const LongCiphertext &x3,
                      const LongCiphertext &x4);

public:
    FFN(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator, sci::NetIO *io, int layer)
        : Protocol(party, encoder, evaluator, io, layer) {}
    ~FFN() {}
    LongCiphertext forward(const LongCiphertext &ln1);
};

#endif
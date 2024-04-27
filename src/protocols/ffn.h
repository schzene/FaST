#ifndef FAST_FFN_H__
#define FAST_FFN_H__
#include "protocol.h"
class FFN : public Protocol {
public:
    FFN(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator,
        IOPack *io_pack) : Protocol(party, encoder, evaluator, io_pack) {}
    ~FFN() {}
    void forward(const LongCiphertext& ln1);
};
#endif
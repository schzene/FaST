#ifndef FAST_LAYER_NROM1_H__
#define FAST_LAYER_NROM1_H__
#include "protocol.h"
class LayerNorm1 : public Protocol {
public:
    LayerNorm1(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator,
               IOPack *io_pack) : Protocol(party, encoder, evaluator, io_pack) {}
    ~LayerNorm1() {}
    LongCiphertext forward(const LongCiphertext &attn, const matrix &input) const;
};
#endif
#ifndef FAST_LAYER_NROM1_H__
#define FAST_LAYER_NROM1_H__
#include "protocol.h"
class LayerNorm : public Protocol {
public:
    LayerNorm(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator,
              sci::NetIO *io, int layer)
        : Protocol(party, encoder, evaluator, io, layer) {}
    ~LayerNorm() {}
    LongCiphertext forward(const LongCiphertext &attn,
                           const matrix &input) const;
};
#endif
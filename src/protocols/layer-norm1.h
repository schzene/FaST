#ifndef FAST_LAYER_NROM1_H__
#define FAST_LAYER_NROM1_H__
#include <module.h>
class LayerNorm1 {
    CKKSKey *party;
    CKKSEncoder *encoder;
    Evaluator *evaluator;
    IOPack *io_pack;

public:
    LayerNorm1(CKKSKey *party_, CKKSEncoder *encoder_, Evaluator *evaluator_,
               IOPack *io_pack_) : party(party_), encoder(encoder_), evaluator(evaluator_),
                                   io_pack(io_pack_) {}
    ~LayerNorm1() {}
    LongCiphertext forward(const LongCiphertext &attn, const std::vector<double> &input) const;
};
#endif
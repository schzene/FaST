
#ifndef FAST_LAYER_NROM1_H__
#define FAST_LAYER_NROM1_H__
#include "fixed-protocol.h"

class FixedLayerNorm : public FixedProtocol {
public:
    FixedLayerNorm(BFVKey *party, BFVParm *parm,
                   FixOp *fix_party, FixOp *fix_public) : FixedProtocol(party, parm, fix_party, fix_public) {}
    ~FixedLayerNorm() {}
    BFVLongCiphertext forward(const BFVLongCiphertext &attn, const bfv_matrix &input) const;
};
#endif
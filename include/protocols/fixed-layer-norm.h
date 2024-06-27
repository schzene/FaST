#ifndef FAST_FIXED_LAYER_NROM1_H__
#define FAST_FIXED_LAYER_NROM1_H__
#include "fixed-protocol.h"

class FixedLayerNorm : public FixedProtocol {
public:
    FixedLayerNorm(BFVKey *party, BFVParm *parm, FixOp *fixop,
                   FixOp *fix_public)
        : FixedProtocol(party, parm, fixop, fix_public) {}
    ~FixedLayerNorm() {}
    BFVLongCiphertext forward(const BFVLongCiphertext &attn,
                              const bfv_matrix &input) const;
};

#endif
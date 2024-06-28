#ifndef FAST_FIXED_ATTENTION_H__
#define FAST_FIXED_ATTENTION_H__
#include "fixed-protocol.h"

class FixedMultiHeadAttention;

class FixedAttention : public FixedProtocol {
    int head;

public:
    friend FixedMultiHeadAttention;
    FixedAttention(BFVKey *party, BFVParm *parm, FixOp *fixop, FixOp *fix_public, int _head)
        : FixedProtocol(party, parm, fixop, fix_public), head(_head) {}
    ~FixedAttention() {}
    bfv_matrix forward(const bfv_matrix &input) const;
};

class FixedMultiHeadAttention {};

#endif
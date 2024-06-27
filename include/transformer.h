#ifndef FAST_TRANSFORMER_H__
#define FAST_TRANSFORMER_H__
#include "protocols.h"

class Encoder {
    int layer;
    Multi_Head_Attention *multi_head_attn;
    LayerNorm *ln1;
    FFN *ffn;
    LayerNorm *ln2;

public:
    Encoder(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator,
            sci::NetIO *io, int layer);
    ~Encoder();
    matrix forward(const matrix &input);
};

class Transformer {
    Encoder **layer;

public:
    Transformer(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator,
                sci::NetIO *io);
    ~Transformer();
    matrix forward(const matrix &input);
};

#endif
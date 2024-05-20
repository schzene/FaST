#ifndef FAST_TRANSFORMER_H__
#define FAST_TRANSFORMER_H__
#include "protocols.h"

class Transformer {
    Multi_Head_Attention *multi_head_attn;
    LayerNorm *ln1;
    FFN *ffn;
    LayerNorm *ln2;

public:
    Transformer(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator, IOPack *io_pack);
    ~Transformer();
    LongCiphertext forward(const matrix &input);
};
#endif
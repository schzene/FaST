#ifndef FAST_TRANSFORMER_H__
#define FAST_TRANSFORMER_H__
#include "protocols.h"

class Transformer {
    Multi_Head_Attention *multi_head_attn;
    LayerNorm1 *ln1;
    FFN *ffn;
    LayerNorm2 *ln2;

public:
    Transformer(CKKSKey *party, SEALContext *context, IOPack *io_pack,
                size_t n_head, size_t d_module, size_t d_k);
    ~Transformer();
    void forward(const std::vector<double> &input);
};
#endif
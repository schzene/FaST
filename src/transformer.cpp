#include "transformer.h"
Transformer::Transformer(CKKSKey *party, SEALContext *context, IOPack *io_pack,
                         size_t n_head, size_t d_module, size_t d_k) {
    this->multi_head_attn = new Multi_Head_Attention(party, context, io_pack, n_head, d_module, d_k);
    this->ln1 = new LayerNorm1();
    this->ffn = new FFN();
    this->ln2 = new LayerNorm2();
}

Transformer::~Transformer() {
    delete multi_head_attn;
    delete ln1;
    delete ffn;
    delete ln2;
}

void Transformer::forward(const std::vector<double> &input) {
    auto output1 = multi_head_attn->forward(input);
    auto output2 = ln1->forward(output1, input);
    // auto output3 = ffn->forward(output2);
    // return ln2->forward(output3);
}
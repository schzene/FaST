#include "transformer.h"
Transformer::Transformer(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator, IOPack *io_pack) {
    this->multi_head_attn = new Multi_Head_Attention(party, encoder, evaluator, io_pack);
    this->ln1 = new LayerNorm1(party, encoder, evaluator, io_pack);
    this->ffn = new FFN(party, encoder, evaluator, io_pack);
    this->ln2 = new LayerNorm2(party, encoder, evaluator, io_pack);
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
    ffn->forward(output2);
    ln2->forward();
}
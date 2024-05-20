#include "transformer.h"
Transformer::Transformer(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator, IOPack *io_pack) {
    this->multi_head_attn = new Multi_Head_Attention(party, encoder, evaluator, io_pack);
    this->ln1 = new LayerNorm(party, encoder, evaluator, io_pack);
    this->ffn = new FFN(party, encoder, evaluator, io_pack);
    this->ln2 = new LayerNorm(party, encoder, evaluator, io_pack);
}

Transformer::~Transformer() {
    delete multi_head_attn;
    delete ln1;
    delete ffn;
    delete ln2;
}

LongCiphertext Transformer::forward(const matrix &input) {
    LongCiphertext output1 = multi_head_attn->forward(input);
    LongCiphertext output2 = ln1->forward(output1, input);
    LongCiphertext output3 = ffn->forward(output2);
    return ln2->forward(output3, input);
}
#include "transformer.h"
Transformer::Transformer(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator, sci::NetIO *io) {

    this->multi_head_attn = new Multi_Head_Attention(party, encoder, evaluator, io);
    this->ln1 = new LayerNorm(party, encoder, evaluator, io);
    this->ffn = new FFN(party, encoder, evaluator, io);
    this->ln2 = new LayerNorm(party, encoder, evaluator, io);
}

Transformer::~Transformer() {
    delete multi_head_attn;
    // add dense layer: ref BOLT Linear2
    delete ln1;
    delete ffn;
    delete ln2;
}

LongCiphertext Transformer::forward(const matrix &input) {
    LongCiphertext output1 = multi_head_attn->forward(input);
    LongCiphertext output2 = ln1->forward(output1, input);
    LongCiphertext output3 = ffn->forward(output2);
    return ln2->forward(output3, input);
    // return output1;
}
#include "transformer.h"

Encoder::Encoder(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator, sci::NetIO *io, int _layer)
    : layer(_layer) {
    this->multi_head_attn = new Multi_Head_Attention(party, encoder, evaluator, io, layer);
    this->ln1 = new LayerNorm(party, encoder, evaluator, io, layer);
    this->ffn = new FFN(party, encoder, evaluator, io, layer);
    this->ln2 = new LayerNorm(party, encoder, evaluator, io, layer);
}

Encoder::~Encoder() {
    delete multi_head_attn;
    // add dense layer: ref BOLT Linear2
    delete ln1;
    delete ffn;
    delete ln2;
}

matrix Encoder::forward(const matrix &input) {
    LongCiphertext output1 = multi_head_attn->forward(input);
    LongCiphertext output2 = ln1->forward(output1, input);
    LongCiphertext output3 = ffn->forward(output2);
    LongCiphertext output4 = ln2->forward(output3, input);
    if (ln2->party->party == sci::ALICE) {
        LongCiphertext out_sec_a;
        LongCiphertext::recv(ln2->io, &out_sec_a, ln2->party->context);
        LongPlaintext out_plain = out_sec_a.decrypt(ln2->party);
        matrix output = out_plain.decode(ln2->encoder);
        return output;
    } else {
        size_t size = input.size();
        matrix neg_output(size);
        random_mat(neg_output);
        LongPlaintext neg_out_plain(neg_output, ln2->encoder);
        neg_out_plain.mod_switch_to_inplace(output4.parms_id(), ln2->evaluator);
        output4.add_plain_inplace(neg_out_plain, ln2->evaluator);
        LongCiphertext::send(ln2->io, &output4);

        matrix output(size);
        for (size_t i = 0; i < size; i++) {
            output[i] = -neg_output[i];
        }
        return output;
    }
}

Transformer::Transformer(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator, sci::NetIO *io) {
    layer = new Encoder *[n_layer];
    for (int i = 0; i < n_layer; i++) {
        layer[i] = new Encoder(party, encoder, evaluator, io, i);
    }
}

Transformer::~Transformer() {
    for (int i = 0; i < n_heads; i++) {
        delete layer[i];
    }
    delete[] layer;
}

matrix Transformer::forward(const matrix &input) {
    matrix output = layer[0]->forward(input);
    for (int i = 1; i < n_heads; i++) {
        output = layer[i]->forward(output);
    }
    return output;
}

#ifndef FAST_LAYER_NROM1_H__
#define FAST_LAYER_NROM1_H__
#include "protocol.h"

class BFVLayerNorm : public BFVProtocol {
public:
    BFVLayerNorm(BFVKey *party, BatchEncoder *encoder, Evaluator *evaluator,
                 IOPack *io_pack) : Protocol(party, encoder, evaluator, io_pack) {}
    ~BFVLayerNorm() {}
    BFVLongCiphertext forward(const BFVLongCiphertext &attn, const bfv_matrix &input) const;
};

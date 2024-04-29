#ifndef FAST_LAYER_NROM2_H__
#define FAST_LAYER_NROM2_H__
#include "protocol.h"
class LayerNorm2 : public Protocol {
public:
    LayerNorm2(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator,
               IOPack *io_pack) : Protocol(party, encoder, evaluator, io_pack) {}
    ~LayerNorm2() {}
    void forward();
};
#endif
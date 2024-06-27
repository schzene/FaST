#ifndef FAST_PROTOCOL_H
#define FAST_PROTOCOL_H
#include "SCI/utils.h" // prg.h & io & arg
#include <model.h>

class Encoder;

class Protocol {
protected:
    int layer;
    CKKSKey *party;
    CKKSEncoder *encoder;
    Evaluator *evaluator;
    sci::NetIO *io;

public:
    friend Encoder;
    Protocol(CKKSKey *party_, CKKSEncoder *encoder_, Evaluator *evaluator_,
             sci::NetIO *io_, int _layer)
        : party(party_), encoder(encoder_), evaluator(evaluator_), io(io_),
          layer(_layer) {}
    ~Protocol() {}
};

#endif
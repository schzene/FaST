#ifndef FAST_PROTOCOL_H
#define FAST_PROTOCOL_H
#include "SCI/utils.h" // prg.h & io & arg
#include <model.h>
#include <string>

class Encoder;

class Protocol {
protected:
    CKKSKey *party;
    CKKSEncoder *encoder;
    Evaluator *evaluator;
    sci::NetIO *io;
    int layer;
    string layer_str;
    string dir_path;

public:
    friend Encoder;
    Protocol(CKKSKey *party_, CKKSEncoder *encoder_, Evaluator *evaluator_, sci::NetIO *io_, int _layer)
        : party(party_), encoder(encoder_), evaluator(evaluator_), io(io_), layer(_layer) {
        layer_str = std::to_string(layer);
        dir_path = party->party == sci::ALICE ? "/data/BOLT/bolt/prune/mrpc/alice_weights_txt/"
                                              : "/data/BOLT/bolt/prune/mrpc/bob_weights_txt/";
    }
    ~Protocol() {}
};

#endif
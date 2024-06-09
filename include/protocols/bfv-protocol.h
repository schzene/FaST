#ifndef FAST_BFV_PROTOCOL_H
#define FAST_BFV_PROTOCOL_H
#include "ezpc_scilib/ezpc_utils.h" // prg.h & io & arg
#include <model.h>
class BFVProtocol {
protected:
    BFVKey *party;
    BatchEncoder *encoder;
    Evaluator *evaluator;
    IOPack *io_pack;

public:
    BFVProtocol(BFVKey *party_, BatchEncoder *encoder_, Evaluator *evaluator_,
                IOPack *io_pack_) : party(party_),
                                    encoder(encoder_),
                                    evaluator(evaluator_),
                                    io_pack(io_pack_) {}
    ~BFVProtocol() {}
};
#endif
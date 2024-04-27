#ifndef FAST_PROTOCOL_H
#define FAST_PROTOCOL_H
#include <module.h>
class Protocol {
protected:
    CKKSKey *party;
    CKKSEncoder *encoder;
    Evaluator *evaluator;
    IOPack *io_pack;

public:
    Protocol(CKKSKey *party_, CKKSEncoder *encoder_, Evaluator *evaluator_,
             IOPack *io_pack_) : party(party_),
                                 encoder(encoder_),
                                 evaluator(evaluator_),
                                 io_pack(io_pack_) {}
    ~Protocol() {}
};
#endif
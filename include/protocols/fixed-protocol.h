#ifndef FAST_BFV_PROTOCOL_H
#define FAST_BFV_PROTOCOL_H
#include "SCI/utils.h" // prg.h & io & arg
#include <model.h>

class FixedProtocol {
protected:
    BFVKey *party;
    BFVParm *parm;
    FixOp *fixop;
    FixOp *fix_public;

public:
    FixedProtocol(BFVKey *party_, BFVParm *parm_, FixOp *fixop_, FixOp *fix_public_)
        : party(party_), parm(parm_), fixop(fixop_), fix_public(fix_public_) {
        assert(party->party == fixop->party);
    }
    ~FixedProtocol() {}
};

#endif
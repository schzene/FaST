#ifndef FAST_BFV_PROTOCOL_H
#define FAST_BFV_PROTOCOL_H
#include "SCI/utils.h" // prg.h & io & arg
#include <model.h>

using sci::ALICE;
using sci::BOB;
using sci::IOPack;
using sci::OTPack;
using sci::PUBLIC;

class FixedProtocol {
protected:
    BFVKey *party;
    BFVParm *parm;
    FixOp *fix_party;
    FixOp *fix_public;

public:
    FixedProtocol(BFVKey *party_, BFVParm *parm_,
                  FixOp *fix_party_, FixOp *fix_public_) : party(party_), parm(parm_),
                                                      fix_party(fix_party_), fix_public(fix_public_) {
        assert(party->party == fix_party->party);
    }
    ~FixedProtocol() {}
};
#endif
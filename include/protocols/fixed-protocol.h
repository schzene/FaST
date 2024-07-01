#ifndef FAST_BFV_PROTOCOL_H
#define FAST_BFV_PROTOCOL_H
#include "SCI/utils.h" // prg.h & io & arg
#include "net_io_channel.h"
#include "utils/conversion.h"
#include <model.h>

#define DEFAULT_SCALE 12
#define DEFAULT_ELL 37

#define GELU_DEFAULT_SCALE 9
#define GELU_DEFAULT_ELL 22

class FixedProtocol {
protected:
    BFVKey *party;
    BFVParm *parm;
    sci::NetIO *io;
    FPMath *fpmath;
    FPMath *fpmath_public;
    Conversion *conv;
    int layer;
    string layer_str;
    string dir_path;

public:
    FixedProtocol(BFVKey *_party, BFVParm *_parm, sci::NetIO *_io, FPMath *_fpmath, FPMath *_fpmath_public,
                  Conversion *_conv, int _layer)
        : party(_party), parm(_parm), io(_io), fpmath(_fpmath), fpmath_public(_fpmath_public), conv(_conv),
          layer(_layer) {
        assert(party->party == fpmath->party);
        layer_str = std::to_string(layer);
        dir_path = party->party == sci::ALICE ? "/data/BOLT/bolt/prune/mrpc/alice_weights_txt/"
                                              : "/data/BOLT/bolt/prune/mrpc/bob_weights_txt/";
    }
    ~FixedProtocol() {}
};

#endif
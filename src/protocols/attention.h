#ifndef FAST_ATTENTION_H__
#define FAST_ATTENTION_H__
#include <utils.h>

class Attention {
    CKKSKey *party;
    CKKSEncoder *encoder;
    SEALContext* context;
    Evaluator *evaluator;
    IOPack* io_pack;
    std::vector<double> input;
    size_t d_module, d_k;
    
public:
    double scale = 1ul<<40;
    Attention(CKKSKey *party_, SEALContext* context, IOPack* io_pack_, std::vector<double>& input_, size_t d_module_, size_t d_k_);
    ~Attention();
    std::vector<double> forward();
};

class Multi_Head_Attention {
public:
    size_t n_head;

};
#endif
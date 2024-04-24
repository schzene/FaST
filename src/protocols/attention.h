#ifndef FAST_ATTENTION_H__
#define FAST_ATTENTION_H__
#include <utils.h>
#define SOFTMAX_TIME_TEST
class Multi_Head_Attention;

class Attention {
    CKKSKey *party;
    CKKSEncoder *encoder;
    SEALContext *context;
    Evaluator *evaluator;
    IOPack *io_pack;
    size_t d_module, d_k, head;

public:
    friend Multi_Head_Attention;
    double scale = 1ul << 40;
    Attention(CKKSKey *party_, SEALContext *context, IOPack *io_pack_,
              size_t d_module_, size_t d_k_, size_t head_);
    ~Attention();
    LongCiphertext forward(const std::vector<double> &input);
};

class Multi_Head_Attention {
public:
    Attention **attns;
    size_t n_head;
    Multi_Head_Attention(CKKSKey *party, SEALContext *context, IOPack *io_pack,
                         size_t n_head_, size_t d_module, size_t d_k);
    ~Multi_Head_Attention();
    LongCiphertext forward(const std::vector<double> &input);
};
#endif
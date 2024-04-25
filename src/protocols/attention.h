#ifndef FAST_ATTENTION_H__
#define FAST_ATTENTION_H__
#include <module.h>
#define SOFTMAX_TIME_TEST
class Multi_Head_Attention;

class Attention {
    CKKSKey *party;
    CKKSEncoder *encoder;
    SEALContext *context;
    Evaluator *evaluator;
    IOPack *io_pack;
    size_t head;

public:
    friend Multi_Head_Attention;
    Attention(CKKSKey *party_, SEALContext *context, IOPack *io_pack_, size_t head_);
    ~Attention();
    std::vector<double> forward(const std::vector<double> &input);
};

class Multi_Head_Attention {
public:
    Attention **attns;
    Multi_Head_Attention(CKKSKey *party, SEALContext *context, IOPack *io_pack);
    ~Multi_Head_Attention();
    LongCiphertext forward(const std::vector<double> &input);
};
#endif
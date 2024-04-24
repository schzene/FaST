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
    std::vector<double> input;
    size_t d_module, d_k, head;

public:
    friend Multi_Head_Attention;
    double scale = 1ul << 40;
    Attention(CKKSKey *party_, SEALContext *context, IOPack *io_pack_, std::vector<double> &input_,
              size_t d_module_, size_t d_k_, size_t head_);
    ~Attention();
    void forward(LongCiphertext &result);
};

class Multi_Head_Attention {
public:
    Attention** attns;
    size_t n_head;
    Multi_Head_Attention(size_t n_head_, CKKSKey *party_, SEALContext *context_, IOPack *io_pack_,
                         std::vector<double> &input_, size_t d_module_, size_t d_k_);
    ~Multi_Head_Attention();
    void forward(LongCiphertext &result);
};
#endif
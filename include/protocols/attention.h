#ifndef FAST_ATTENTION_H__
#define FAST_ATTENTION_H__
#include "protocol.h"

// #define SOFTMAX_TIME_TEST
class Multi_Head_Attention;

class Attention : public Protocol {
    int head;

public:
    friend Multi_Head_Attention;
    Attention(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator, sci::NetIO *io,
              size_t head_) : Protocol(party, encoder, evaluator, io), head(head_) {}
    ~Attention() {}
    matrix forward(const matrix &input) const;
    // std::vector<double> forward(const std::vector<double> &input) const;
};

class Multi_Head_Attention : public Protocol {
    Attention **attns;

public:
    Multi_Head_Attention(CKKSKey *party, CKKSEncoder *encoder, Evaluator *evaluator, sci::NetIO *io);
    ~Multi_Head_Attention();
    LongCiphertext forward(const std::vector<double> &input) const;
};
#endif
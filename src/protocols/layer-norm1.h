#ifndef FAST_LAYER_NROM1_H__
#define FAST_LAYER_NROM1_H__
#include <module.h>
class LayerNorm1 {
public:
    LayerNorm1();
    const std::vector<double> forward(const LongCiphertext &attn, const std::vector<double> &input);
};
#endif
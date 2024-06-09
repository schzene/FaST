#include "bfv-layer-norm.h"

BFVLongCiphertext BFVLayerNorm::(const BFVLongCiphertext &attn, const bfv_matrix &input) const {

    sci::PRG128 prg;
    size_t i, j;

    if (party->party == ALICE) {
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif
        uint64_t *ha1 = new uint64_t[batch_size * d_module];
        uint64_t *ha2 = new uint64_t[batch_size * d_module];
        prg.random_mod_p<uint64_t>(ha1, batch_size * d_module, bfv_plain_mod);
        prg.random_mod_p<uint64_t>(ha2, batch_size * d_module, bfv_plain_mod);

        bfv_matrix ha1_xa(input.size());

        for (size_t i = 0; i < batch_size * d_module; i++) {
            ha1_xa[i] = ModMult(ha1[i], input[i], bfv_plain_mod);
        }

        BFVLongCiphertext ha2_div_ha1_secret_a()
    }
}
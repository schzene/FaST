#include "fixed-layer-norm.h"

BFVLongCiphertext FixedLayerNorm::forward(const BFVLongCiphertext &attn, const bfv_matrix &input) const {

    sci::PRG128 prg;
    size_t i, j;

    if (party->party == ALICE) {
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif
        uint64_t ha1, ha2, ka;
        prg.random_mod_p<uint64_t>(&ha1, 1, party->parm->plain_mod);
        prg.random_mod_p<uint64_t>(&ha2, 1, party->parm->plain_mod);
        prg.random_mod_p<uint64_t>(&ka, 1, party->parm->plain_mod);
        FixArray fix_input = fix_party->input(party->party, input.size(), input.data());
        BFVLongPlaintext ha2_plain(parm, ha2);
        BFVLongCiphertext ha2_div_ha1_secret_a(parm, ha2 / ha1, party), ha2_secret_a(ha2_plain, party), xha1_secret_a;


        FixArray fixed_ha1_xa = fix_party->mul(fix_input, ha1), fixed_ha1_xa_trunc = fix_party->truncate_reduce(fixed_ha1_xa);
        fixed_ha1_xa_trunc.party = sci::PUBLIC;
        BFVLongCiphertext attn_ha2_b = attn.multiply_plain(ha2_plain, party->parm->evaluator);
        
    }
}
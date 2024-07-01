#include "conversion.h"

void Conversion::Ring_to_Prime(uint64_t input, uint64_t output, int ell, int64_t plain_mod) {
#ifdef LOG
    auto t_conversion = high_resolution_clock::now();
#endif
    output = sci::neg_mod(sci::signed_val(input, ell), (int64_t)plain_mod);

#ifdef LOG
    t_total_conversion += interval(t_conversion);
#endif
}

// R-to-P:location conversion
void Conversion::Ring_to_Prime(const uint64_t *input, uint64_t *output, int length, int ell, int64_t plain_mod) {
#ifdef LOG
    auto t_conversion = high_resolution_clock::now();
#endif

    vector<uint64_t> tmp(length);
    for (size_t i = 0; i < length; i++) {
        tmp[i] = sci::neg_mod(sci::signed_val(input[i], ell), (int64_t)plain_mod);
    }
    memcpy(output, tmp.data(), length * sizeof(uint64_t));

#ifdef LOG
    t_total_conversion += interval(t_conversion);
#endif
}
// P-to-R
void Conversion::Prime_to_Ring(int party, const uint64_t *input, uint64_t *output, int length, int ell,
                               int64_t plain_prime, int s_in, int s_out, FPMath *fpmath) {
    // if input > plain_prime, then sub plain_prime
    // sub plain_prime/2 anyway

    FixArray fix_input = fpmath->fix->input(party, length, input, true, ell, s_in);
    FixArray p_array = fpmath->fix->input(sci::PUBLIC, length, plain_prime, true, ell, s_in);
    FixArray p_2_array = fpmath->fix->input(sci::PUBLIC, length, (plain_prime - 1) / 2, true, ell, s_in);
    FixArray tmp = fpmath->gt_p_sub(fix_input, p_array);
    tmp = fpmath->fix->sub(tmp, p_2_array);

    if (s_in > s_out) {
        tmp = fpmath->fix->right_shift(tmp, s_in - s_out);
    } else if (s_in < s_out) {
        tmp = fpmath->fix->mul(tmp, 1 << (s_out - s_in));
    }

    memcpy(output, tmp.data, length * sizeof(uint64_t));
}

// P-to-R:location conversion
void Conversion::Prime_to_Ring(const uint64_t *input, uint64_t *output, int length, int ell, int64_t plain_prime,
                               int s_in, int s_out, FPMath *fpmath) {
    FixArray fix_input = fpmath->fix->input(sci::PUBLIC, length, input, true, ell, s_in);
    FixArray p_array = fpmath->fix->input(sci::PUBLIC, length, plain_prime, true, ell, s_in);
    FixArray p_2_array = fpmath->fix->input(sci::PUBLIC, length, (plain_prime - 1) / 2, true, ell, s_in);
    FixArray tmp = fpmath->location_gt_p_sub(fix_input, p_2_array);
    tmp = fpmath->fix->sub(tmp, p_2_array);
    if (s_in > s_out) {
        tmp = fpmath->fix->location_right_shift(tmp, s_in - s_out);
    } else if (s_in < s_out) {
        tmp = fpmath->fix->mul(tmp, 1 << (s_out - s_in));
    }

    memcpy(output, tmp.data, length * sizeof(uint64_t));
}

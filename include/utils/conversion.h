#ifndef FAST_CONVERSION_H__
#define FAST_CONVERSION_H__

#include "vector"
#include "he-bfv.h"
#include "SCI/utils.h"

class Conversion
{
public:
    Conversion() {}

    ~Conversion() {}

    void Prime_to_Ring(int party, const uint64_t *input, uint64_t *output, int length, int ell, int64_t plain_prime, int s_in, int s_out, FPMath *fpmath);

    void Prime_to_Ring(const uint64_t *input, uint64_t *output, int length, int ell, int64_t plain_prime, int s_in, int s_out, FPMath *fpmath);

    void Ring_to_Prime(const uint64_t *input, uint64_t *output, int length, int ell, int64_t plain_mod);

    void Ring_to_Prime(const uint64_t input, uint64_t output, int ell, int64_t plain_mod);
};

#endif // FAST_HE_BFV_CONVERSION_H__
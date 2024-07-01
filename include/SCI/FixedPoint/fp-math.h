#ifndef FIXED_POINT_MATH_H__
#define FIXED_POINT_MATH_H__
#include "fixed-point.h"
#include "Math/math-functions.h"
#include <cmath>
class FPMath
{
public:
    int party;
    sci::IOPack *iopack;
    sci::OTPack *otpack;
    BoolOp *bool_op;
    FixOp *fix;
    MathFunctions *math;

    FPMath(int party, sci::IOPack *iopack, sci::OTPack *otpack)
    {
        this->party = party;
        this->iopack = iopack;
        this->otpack = otpack;
        this->bool_op = new BoolOp(party, iopack, otpack);
        this->fix = new FixOp(party, iopack, otpack);
        this->math = new MathFunctions(party, iopack, otpack);
    }

    ~FPMath()
    {
        delete bool_op;
        delete fix;
        delete math;
    }

    // Fixed-Point Math Functions: returns OP(x[i]), OP = {sinpi, cospi, tanpi, exp2, log2, exp, ln, erf}
    // x must be secret-shared

    std::tuple<FixArray, FixArray> exp4(const FixArray &x);

    FixArray lookup_table_exp(const FixArray &x);

    FixArray tanh_inner(const FixArray &x);

    FixArray tanh_inner_preprocess(const FixArray &x);

    FixArray tanh_approx(const FixArray &x);

    FixArray gt_p_sub(const FixArray &x, const FixArray &p);

    FixArray location_gt_p_sub(const FixArray &x, const FixArray &p);

    FixArray sqrt_(const FixArray &x, bool recp_sqrt);

    std::tuple<FixArray, FixArray, FixArray> bitonic_sort_and_swap(
        const FixArray &x, FixArray softmax_v_, FixArray h1_, bool swap);

    void print(const FixArray &x);

    vector<FixArray> mean(const vector<FixArray> &x);

    vector<FixArray> standard_deviation(const vector<FixArray> &x, const vector<FixArray> mean);

    double sqrt_(float x);
};

#endif // FIXED_POINT_MATH_H__
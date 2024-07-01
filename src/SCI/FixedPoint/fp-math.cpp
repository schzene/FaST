#include "fp-math.h"
#include "fp-math-coeffs.h"

using namespace sci;

#define FRAC_RANGE 9
#define FP_INTMD_M_BITS 27
#define FP_INTMD_E_BITS 8
#define PI_DOUBLE \
    3.1415926535897932384626433832795028841971693993751058209749445923078164062
#define LOG2E \
    1.44269504088896340735992468100189213742664595415298593413544940693110921918118507988552662289350634449699
#define LOGE2 \
    0.693147180559945309417232121458176568075500134360255254120680009493393621969694715605863326996418687
#define TWO_INV_SQRT_PI \
    1.128379167095512573896158903121545171688101258657997713688171443421284936882
#define NEG_LOGE2_INV \
    1.442695040888963423535598661526235116567603930130965898132921686199121361438241594804331289503955470328942932380383923264

FixArray get_idx_from_input(FixOp *fix, const FixArray &delta_m,
                            const FixArray &delta_e, int idx_m_bits,
                            int idx_e_bits, int e_offset)
{
    assert(delta_m.party != PUBLIC && delta_e.party != PUBLIC);
    assert(delta_m.size == delta_e.size);
    assert(idx_m_bits + idx_e_bits <= delta_e.ell);
    FixArray idx_hi =
        fix->reduce(fix->add(delta_e, e_offset), idx_m_bits + idx_e_bits);
    idx_hi.signed_ = false;
    if (idx_m_bits == 0)
    {
        return idx_hi;
    }
    idx_hi = fix->mul(idx_hi, 1 << idx_m_bits, idx_m_bits + idx_e_bits);
    FixArray idx_lo = fix->truncate_reduce(delta_m, delta_m.ell - 1 - idx_m_bits);
    idx_lo = fix->sub(idx_lo, 1 << idx_m_bits);
    if (idx_m_bits + idx_e_bits < idx_m_bits + 1)
    {
        idx_lo = fix->reduce(idx_lo, idx_m_bits + idx_e_bits);
    }
    idx_lo.s = 0;
    BoolArray all_0 = fix->bool_op->input(ALICE, delta_m.size, uint8_t(0));
    FixArray idx = fix->add(
        idx_hi, fix->extend(idx_lo, idx_m_bits + idx_e_bits, all_0.data));
    return idx;
}

std::tuple<FixArray, FixArray> FPMath::exp4(const FixArray &x)
{

    /*
    l = np.floor((x / -math.log(2)))
    p = x + l*math.log(2)
    fp = poly(p)
    return fp / (2**l)
    */

    // print_fix(x);

    int ell = x.ell;
    int scale = x.s;

    // All 0 and all 1 array for msb arg
    BoolArray all_0 = bool_op->input(ALICE, x.size, uint8_t(0));
    BoolArray all_1 = bool_op->input(ALICE, x.size, 1);

    // ln2
    FixArray ln2 = fix->input(PUBLIC, x.size, uint64_t(2839), true, ell, scale);
    // print_fix(ln2);

    // inverse of negative ln2
    FixArray inl = fix->input(PUBLIC, x.size, uint64_t(-5909), true, ell, scale);
    // print_fix(inl);

    // x / -math.log(2)
    // Truncate to original scale and bitlength
    FixArray x_inl = fix->mul(x, inl, ell + scale);
    // Optimization: local truncation
    x_inl = fix->truncate_reduce(x_inl, scale);
    // x_inl =  fix->reduce(x_inl, ell);
    // print_fix(x_inl);

    // Get the integer part and scale back
    FixArray l_short = fix->truncate_reduce(x_inl, scale);
    FixArray l_short_raw = l_short;
    FixArray l = fix->scale_up(l_short, ell, scale);

    // l*math.log(2)
    FixArray l_ln2 = fix->mul(l, ln2, ell + scale, all_0.data, all_0.data);
    l_ln2 = fix->truncate_reduce(l_ln2, scale);
    // l_ln2 =  fix->reduce(l_ln2, ell);

    // Get the decimal part
    FixArray p = fix->add(x, l_ln2);
    // Optimization: We don't need that much bit as p \in (-ln2, 0])
    p = fix->reduce(p, scale + 2);

    // Polynomial fit
    FixArray poly_p = fix->poly1(p);
    poly_p = fix->extend(poly_p, ell, all_0.data);

    l_short.signed_ = false;
    // Optimization: The polynomial result is within [0, ~0.7)
    // Thus the upper bound of shift is scale + 1

    FixArray bound = fix->input(PUBLIC, l_short.size, 13, false, l_short.ell, 0);
    BoolArray gt_bound = fix->GT(l_short, bound);
    l_short = fix->if_else(gt_bound, bound, l_short);

    FixArray ret = fix->right_shift(poly_p, l_short, scale + 1, all_1.data);

    return std::make_tuple(ret, l_short_raw);
}

FixArray FPMath::lookup_table_exp(const FixArray &x)
{
    FixArray ret(party, x.size, x.signed_, x.ell, x.s);
    math->lookup_table_exp(x.size, x.data, ret.data, x.ell, x.ell, x.s, x.s);
    return ret;
}

FixArray FPMath::tanh_inner_preprocess(const FixArray &x)
{
    int N = x.size;
    int ell = x.ell;
    int s = x.s;

    BoolArray all_0 = bool_op->input(ALICE, N, uint8_t(0));
    BoolArray all_1 = bool_op->input(ALICE, N, 1);

    // Const
    FixArray t0 = fix->input(PUBLIC, x.size, uint64_t((-4.259314087994767) * (1 << s)), true, ell, s);
    FixArray t1 = fix->input(PUBLIC, x.size, uint64_t((18.86353816972803) * (1 << s)), true, ell, s);
    FixArray t2 = fix->input(PUBLIC, x.size, uint64_t((-36.42402897526823) * (1 << s)), true, ell, s);
    FixArray t3 = fix->input(PUBLIC, x.size, uint64_t((-0.013232131886235352) * (1 << s)), true, ell, s);
    FixArray t4 = fix->input(PUBLIC, x.size, uint64_t((-3.3289339650097993) * (1 << s)), true, ell, s);
    FixArray t5 = fix->input(PUBLIC, x.size, uint64_t((-0.0024920889620412097) * (1 << s)), true, ell, s);

    // p1(x) = (x + t0)*x + t1
    // Range: >0
    FixArray p1 = fix->add(x, t0);
    p1 = fix->mul(p1, x, ell + s, all_0.data, all_0.data);
    p1 = fix->truncate_reduce(p1, s);
    p1 = fix->add(p1, t1);

    // p2(x) = (p1(x) + x + t2)*p1(x)*x*t3 + t4*x + t5

    // (p1(x) + x + t2) < 0
    FixArray p2 = fix->add(p1, x);
    p2 = fix->add(p2, t2);

    // (p1(x) + x + t2)*p1(x) < 0
    p2 = fix->mul(p2, p1, ell + s, all_1.data, all_0.data);
    p2 = fix->truncate_reduce(p2, s);

    // p2(x) = (p1(x) + x + t2)*p1(x)*t3 >0
    p2 = fix->mul(p2, t3, ell + s, all_1.data, all_1.data);
    p2 = fix->truncate_reduce(p2, s);

    // p2(x) = (p1(x) + x + t2)*p1(x)*x*t3 < 0
    p2 = fix->mul(p2, x, ell + s, all_1.data, all_0.data);
    p2 = fix->truncate_reduce(p2, s);

    FixArray t4x = fix->mul(x, t4, ell + s, all_0.data, all_1.data);
    t4x = fix->truncate_reduce(t4x, s);

    p2 = fix->add(p2, t4x);
    p2 = fix->add(p2, t5);

    return p2;
}

FixArray FPMath::tanh_inner(const FixArray &x)
{
    int N = x.size;
    int ell = x.ell;
    int s = x.s;

    // Const
    FixArray a = fix->input(PUBLIC, x.size, uint64_t((-0.013232131886235352) * (1 << s)), true, ell, s);
    FixArray b = fix->input(PUBLIC, x.size, uint64_t((0.09948747962825866) * (1 << s)), true, ell, s);
    FixArray c = fix->input(PUBLIC, x.size, uint64_t((-0.20093640347818847) * (1 << s)), true, ell, s);
    FixArray d = fix->input(PUBLIC, x.size, uint64_t((-0.17616532856475706) * (1 << s)), true, ell, s);
    FixArray e = fix->input(PUBLIC, x.size, uint64_t((1.0542492677156243) * (1 << s)), true, ell, s);
    FixArray f = fix->input(PUBLIC, x.size, uint64_t((-0.0024920889620412097) * (1 << s)), true, ell, s);

    BoolArray all_0 = bool_op->input(ALICE, N, uint8_t(0));
    BoolArray all_1 = bool_op->input(ALICE, N, 1);

    //
    FixArray x_square = fix->mul(x, x, ell + s, all_0.data, all_0.data);
    x_square = fix->truncate_reduce(x_square, s);

    FixArray x_cube = fix->mul(x_square, x, ell + s, all_0.data, all_0.data);
    x_cube = fix->truncate_reduce(x_cube, s);

    FixArray x_four = fix->mul(x_square, x_square, ell + s, all_0.data, all_0.data);
    x_four = fix->truncate_reduce(x_four, s);

    FixArray x_five = fix->mul(x_four, x, ell + s, all_0.data, all_0.data);
    x_five = fix->truncate_reduce(x_five, s);

    FixArray x_five_a = fix->mul(x_five, a, ell + s, all_0.data, all_1.data);
    x_five_a = fix->truncate_reduce(x_five_a, s);

    FixArray x_four_b = fix->mul(x_four, b, ell + s, all_0.data, all_0.data);
    x_four_b = fix->truncate_reduce(x_four_b, s);

    FixArray x_cube_c = fix->mul(x_cube, c, ell + s, all_0.data, all_1.data);
    x_cube_c = fix->truncate_reduce(x_cube_c, s);

    FixArray x_square_d = fix->mul(x_square, d, ell + s, all_0.data, all_1.data);
    x_square_d = fix->truncate_reduce(x_square_d, s);

    FixArray x_e = fix->mul(x, e, ell + s, all_0.data, all_0.data);
    x_e = fix->truncate_reduce(x_e, s);

    f = fix->add(f, x_e);
    f = fix->add(f, x_square_d);
    f = fix->add(f, x_cube_c);
    f = fix->add(f, x_four_b);
    f = fix->add(f, x_five_a);

    return f;
}

FixArray FPMath::tanh_approx(const FixArray &x)
{
    int N = x.size;
    int ell = x.ell;
    int s = x.s;

    BoolArray all_0 = bool_op->input(ALICE, N, uint8_t(0));
    BoolArray all_1 = bool_op->input(ALICE, N, 1);

    FixArray cons_2 = fix->input(PUBLIC, x.size, uint64_t((2 << s)), true, ell, s);
    FixArray cons_1 = fix->input(PUBLIC, x.size, uint64_t((1 << s)), true, ell, s);
    FixArray cons_neg_1 = fix->input(PUBLIC, x.size, uint64_t((-1 << s)), true, ell, s);

    BoolArray pos = fix->GT(x, 0);
    FixArray neg_x = fix->mul(x, -1);
    FixArray abs_x = fix->if_else(pos, x, neg_x);

    FixArray cond_fix = fix->B2A(pos, true, ell);
    cond_fix = fix->scale_up(cond_fix, ell, s);
    FixArray sign_x = fix->mul(cond_fix, cons_2, ell + s, all_0.data, all_0.data);
    sign_x = fix->truncate_reduce(sign_x, s);
    sign_x = fix->add(sign_x, cons_neg_1);

    BoolArray gt3 = fix->GT(abs_x, (uint64_t)(2.855 * (1 << s)));
    FixArray abs_tanh = fix->if_else(gt3, cons_1, tanh_inner_preprocess(abs_x));
    FixArray ret = fix->mul(abs_tanh, sign_x, ell + s, all_0.data);
    ret = fix->truncate_reduce(ret, s);

    return ret;
}

FixArray FPMath::sqrt_(const FixArray &x, bool recp_sqrt)
{
    FixArray ret(party, x.size, x.signed_, x.ell, x.s);
    math->sqrt(x.size, x.data, ret.data, x.ell, x.ell, x.s, x.s, recp_sqrt);
    return ret;
}

FixArray FPMath::gt_p_sub(const FixArray &x, const FixArray &p)
{
    BoolArray gt = fix->GT(x, p);
    FixArray sub = fix->sub(x, p);
    return fix->if_else(gt, sub, x);
}

FixArray FPMath::location_gt_p_sub(const FixArray &x, const FixArray &p)
{
    BoolArray gt = fix->location_GT(x, p);
    FixArray sub = fix->sub(x, p);
    return fix->location_if_else(gt, sub, x); // use location_if_else, without mul multiplexer.
}

void FPMath::print(const FixArray &x)
{
    print_fix(x);
}

BoolArray bitonic_reverse(const BoolArray &x, int array_size, int cur_depth)
{
    BoolArray ret(x.party, x.size);
    int block_size = 2 * cur_depth;
    int num_block = array_size / block_size;
    for (int i = 0; i < num_block; i++)
    {

        for (int j = 0; j < cur_depth; j++)
        {
            int index = i * cur_depth + j;
            if (i % 2 == 1)
            {
                ret.data[index] = x.data[index] ^ ((x.party != BOB) ? 1 : 0);
            }
            else
            {
                ret.data[index] = x.data[index];
            }
        }
    }
    return ret;
}

std::tuple<FixArray, FixArray, FixArray> FPMath::bitonic_sort_and_swap(
    const FixArray &x_, FixArray softmax_v_, FixArray h1_, bool swap)
{
    FixArray x = x_;
    FixArray softmax_v = softmax_v_;
    FixArray h1 = h1_;

    int array_size = x.size;
    int max_depth = array_size / 2;
    int cur_depth = 1;

    int common_dim;

    while (cur_depth <= max_depth)
    {
        int cur_iter = cur_depth;
        while (cur_iter > 0)
        {
            int block_size = 2 * cur_iter;
            int num_block = array_size / block_size;

            vector<int> index_left;
            vector<int> index_right;

            FixArray array_left(party, x.size / 2, x.signed_, x.ell, x.s);
            FixArray array_right(party, x.size / 2, x.signed_, x.ell, x.s);
            FixArray array_reverse(party, x.size, x.signed_, x.ell, x.s);

            FixArray softmax_v_reverse;
            FixArray h1_reverse;

            if (swap)
            {
                common_dim = softmax_v.size / x.size;
                assert(common_dim == 768);

                softmax_v_reverse = fix->input(party, softmax_v.size, (uint64_t)0, softmax_v.signed_, softmax_v.ell, softmax_v.s);
                h1_reverse = fix->input(party, h1.size, (uint64_t)0, h1.signed_, h1.ell, h1.s);

                for (int i = 0; i < num_block; i++)
                {
                    for (int j = 0; j < cur_iter; j++)
                    {
                        int pos_x = i * block_size + j;
                        int pos_y = i * block_size + j + cur_iter;
                        index_left.push_back(pos_x);
                        index_right.push_back(pos_y);
                        array_reverse.data[pos_x] = x.data[pos_y];
                        array_reverse.data[pos_y] = x.data[pos_x];

                        memcpy(
                            &softmax_v_reverse.data[pos_x * common_dim],
                            &softmax_v.data[pos_y * common_dim],
                            common_dim * sizeof(uint64_t));

                        memcpy(
                            &softmax_v_reverse.data[pos_y * common_dim],
                            &softmax_v.data[pos_x * common_dim],
                            common_dim * sizeof(uint64_t));

                        memcpy(
                            &h1_reverse.data[pos_x * common_dim],
                            &h1.data[pos_y * common_dim],
                            common_dim * sizeof(uint64_t));

                        memcpy(
                            &h1_reverse.data[pos_y * common_dim],
                            &h1.data[pos_x * common_dim],
                            common_dim * sizeof(uint64_t));
                    }
                }
            }
            else
            {
                for (int i = 0; i < num_block; i++)
                {
                    for (int j = 0; j < cur_iter; j++)
                    {
                        index_left.push_back(i * block_size + j);
                        index_right.push_back(i * block_size + j + cur_iter);
                        array_reverse.data[i * block_size + j] = x.data[i * block_size + j + cur_iter];
                        array_reverse.data[i * block_size + j + cur_iter] = x.data[i * block_size + j];
                    }
                }
            }

            for (int i = 0; i < array_size / 2; i++)
            {
                array_left.data[i] = x.data[index_left[i]];
                array_right.data[i] = x.data[index_right[i]];
            }

            // print_fix(array_left);
            // print_fix(array_right);
            // print_fix(array_reverse);

            BoolArray lt = fix->LT(array_left, array_right);
            BoolArray cmp_extend = BoolArray(party, lt.size * 2);

            // Reverse some comparisons
            BoolArray cmp = bitonic_reverse(lt, array_size, cur_depth);
            for (int i = 0; i < num_block; i++)
            {
                for (int j = 0; j < cur_iter; j++)
                {
                    cmp_extend.data[i * block_size + j] =
                        cmp.data[i * cur_iter + j];
                    cmp_extend.data[i * block_size + j + cur_iter] =
                        cmp.data[i * cur_iter + j];
                }
            }
            // print_bool(lt);
            // print_bool(cmp);
            // print_bool(cmp_extend);
            // assert(0);

            // print_fix(x);
            x = fix->if_else(cmp_extend, x, array_reverse);
            // print_fix(x);

            if (swap)
            {
                BoolArray cmp_flat = BoolArray(party, cmp_extend.size * common_dim);
                for (int i = 0; i < cmp_flat.size; i++)
                {
                    cmp_flat.data[i] = cmp_extend.data[i / common_dim];
                }

                softmax_v = fix->if_else(cmp_flat, softmax_v, softmax_v_reverse);
                h1 = fix->if_else(cmp_flat, h1, h1_reverse);
            }

            cur_iter /= 2;
        }
        cur_depth *= 2;
    }

    return std::make_tuple(x, softmax_v, h1);
}

double FPMath::sqrt_(float x)
{
    return sqrt(x);
}

// math function for FASTLMPI
vector<FixArray> FPMath::mean(const vector<FixArray> &x)
{
    int party_origin = x[0].party;
    int N = x.size();
    int n = x[0].size;
    int ell = x[0].ell;
    int s = x[0].s;
    bool signed_ = x[0].signed_;
    FixArray sum_res = fix->tree_sum(x);
    uint64_t dn = static_cast<uint64_t>((1.0 / n) * (1ULL << s));
    FixArray fix_dn = fix->input(sci::PUBLIC, N, dn, true, ell, s);
    sum_res.party = sci::ALICE;
    FixArray avg = fix->mul(sum_res, fix_dn, ell);
    avg.party = sci::PUBLIC;
    avg = fix->location_truncation(avg, s);
    avg.party = party_origin;
    vector<FixArray> ret(N);
    for (int i = 0; i < N; i++)
    {
        ret[i] = FixArray(party_origin, 1, signed_, ell, s);
        memcpy(ret[i].data, &avg.data[i], sizeof(uint64_t));
    }

    return ret;
}

vector<FixArray> FPMath::standard_deviation(const vector<FixArray> &x, const vector<FixArray> mean)
{
    int party_origin = x[0].party;
    int N = x.size();
    int n = x[0].size;
    int ell = x[0].ell;
    int s = x[0].s;
    bool signed_ = x[0].signed_;

    uint64_t dn = uint64_t(((1.0 / n) * pow(2, s)));
    FixArray fix_dn = fix->input(sci::PUBLIC, N, dn, true, ell, s);
    vector<FixArray> tmp_ret(N);
    vector<FixArray> tmp_y(N);
    for (size_t i = 0; i < N; i++)
    {
        tmp_ret[i] = fix->sub(x[i], mean[i].data[0]);
        tmp_ret[i] = fix->public_mul(tmp_ret[i], tmp_ret[i], ell + 2 * s);
        tmp_ret[i] = fix->location_truncation(tmp_ret[i], s);
    }

    FixArray sum_res = fix->tree_sum(tmp_ret); // obtain (((g)^s)^2delta)^2
    sum_res.party = sci::ALICE;
    FixArray avg = fix->mul(sum_res, fix_dn, ell);
    avg.party = sci::PUBLIC;
    avg = fix->location_truncation(avg, s);
    uint64_t unsig_fix_delta;
    vector<FixArray> ret(N);
    for (size_t i = 0; i < N; i++)
    {
        ret[i] = FixArray(party_origin, 1, signed_, ell, s);
        double delta = double(avg.data[i]) / (1ULL << s);
        unsig_fix_delta = static_cast<int64_t>(1.0 / sqrt_(float(delta)) * (1ULL << s)); // (-5, 5)
        unsig_fix_delta = sci::neg_mod(unsig_fix_delta, (int64_t)(1ULL << ell));         // (0, 10)
        ret[i].data[0] = unsig_fix_delta;
    }
    return ret;
}
#include <model.h>

#include "SCI/utils.h"
#define TEST
#define DEFAULT_SCALE 12
#define DEFAULT_ELL 37

class SecureLayerNorm1
{
    BFVParm *bfv_parm;
    BFVKey *alice;
    BFVKey *bob;

    sci::IOPack *iopack;
    sci::OTPack *otpack;

    FPMath *fpmath_alice;
    FPMath *fpmath_bob;
    FPMath *fpmath_public;
    FixOp *fix;
    BoolOp *bool_op;
    Conversion *conv;

public:
    SecureLayerNorm1(BFVParm *bfv_parm_, BFVKey *alice_, BFVKey *bob_,
                     sci::IOPack *iopack_, sci::OTPack *otpack_) : bfv_parm(bfv_parm_),
                                                                   alice(alice_),
                                                                   bob(bob_),
                                                                   iopack(iopack_),
                                                                   otpack(otpack_)
    {
        fpmath_alice = new FPMath(sci::ALICE, iopack, otpack);
        fpmath_bob = new FPMath(sci::BOB, iopack, otpack);
        fpmath_public = new FPMath(sci::PUBLIC, iopack, otpack);
        conv = new Conversion();
        // fix = new FixOp(sci::PUBLIC, iopack, otpack);
        // bool_op = new BoolOp(sci::PUBLIC, iopack, otpack);
    }

    ~SecureLayerNorm1()
    {
        delete fpmath_alice;
        delete fpmath_bob;
        delete fpmath_public;
        delete conv;
        delete fix;
        delete bool_op;
    }

    void forward(BFVLongCiphertext &attn_b, uint64_t *input_a, uint64_t *input_b)
    {

        sci::PRG128 prg;
        size_t i, j;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(0, 1);

        /*
            alice generate ha
            compute xa_ha,  [ha], attn_sec_b_ ha
        */
        double ha = dist(gen);

        FixArray fix_ha = fpmath_alice->fix->input(sci::ALICE, batch_size * d_module,
                                                   (sci::neg_mod(static_cast<int64_t>(ha * (1ULL << (DEFAULT_SCALE))), DEFAULT_ELL)),
                                                   true, DEFAULT_ELL, DEFAULT_SCALE);

        FixArray fix_div_ha = fpmath_alice->fix->input(sci::ALICE, batch_size * d_module,
                                                       (sci::neg_mod(static_cast<int64_t>((1.0 / ha) * (1ULL << (DEFAULT_SCALE))), DEFAULT_ELL)),
                                                       true, DEFAULT_ELL, DEFAULT_SCALE);

        FixArray fix_xa = fpmath_alice->fix->input(sci::ALICE, batch_size * d_module,
                                                   input_a,
                                                   true, DEFAULT_ELL, DEFAULT_SCALE);

        fix_ha.party = sci::PUBLIC;
        FixArray fix_ha_xa = fpmath_alice->fix->mul(fix_xa, fix_ha, DEFAULT_ELL);
        fix_ha_xa = fpmath_alice->fix->location_truncation(fix_ha_xa, DEFAULT_SCALE);

        uint64_t *prime_ha_xa = new uint64_t[batch_size * d_module];
        uint64_t *prime_ha = new uint64_t[batch_size * d_module];
        conv->Ring_to_Prime(fix_ha_xa.data, prime_ha_xa, batch_size * d_module, DEFAULT_ELL, bfv_parm->plain_mod);
        conv->Ring_to_Prime(fix_ha.data, prime_ha, batch_size * d_module, DEFAULT_ELL, bfv_parm->plain_mod);

        BFVLongPlaintext ha_plain(bfv_parm, prime_ha_xa, batch_size * d_module);
        BFVLongCiphertext ha_secret_a(ha_plain, alice);
        BFVLongCiphertext attn_ha_secret_b = attn_b.multiply_plain(ha_plain, bfv_parm->evaluator);
        attn_ha_secret_b.mod_switch_to_next_inplace(bfv_parm->evaluator);

        // Alice : send H1 = {ha_xa, ha_secret_a, attn_ha_secret_b} to bob

        // Bob: receive H1, and get x_b[ha]_a, attn_ha
        // computate: attn_ha + xb[ha]_a + xaha
        // get [X_{add}ha]_a

        BFVLongPlaintext attn_ha_plain = attn_ha_secret_b.decrypt(bob);
        uint64_t *prime_xb = new uint64_t[batch_size * d_module];

        conv->Ring_to_Prime(input_b, prime_xb, batch_size * d_module, DEFAULT_ELL, bfv_parm->plain_mod);

        BFVLongPlaintext xb_plain(bfv_parm, prime_xb, batch_size * d_module);
        BFVLongCiphertext xb_ha_secret_a = ha_secret_a.multiply_plain(xb_plain, bfv_parm->evaluator); // ha_xb
        xb_ha_secret_a.mod_switch_to_next_inplace(bfv_parm->evaluator);
        xb_ha_secret_a.add_plain_inplace(attn_ha_plain, bfv_parm->evaluator);
        BFVLongPlaintext ha_xa_plain(bfv_parm, prime_ha_xa, batch_size * d_module);
        xb_ha_secret_a.add_plain_inplace(ha_xa_plain, bfv_parm->evaluator);

        // Bob generate gb, get [x_add*ha*gb]_a

        double gb = dist(gen);

        FixArray fix_gb = fpmath_alice->fix->input(sci::ALICE, batch_size * d_module,
                                                   (sci::neg_mod(static_cast<int64_t>(gb * (1ULL << (DEFAULT_SCALE))), DEFAULT_ELL)),
                                                   true, DEFAULT_ELL, DEFAULT_SCALE);
        uint64_t *prime_gb = new uint64_t[batch_size * d_module];
        conv->Ring_to_Prime(fix_gb.data, prime_gb, batch_size * d_module, DEFAULT_ELL, bfv_parm->plain_mod);
        BFVLongPlaintext gb_plain(bfv_parm, prime_gb, batch_size * d_module);

        xb_ha_secret_a.multiply_plain_inplace(gb_plain, bfv_parm->evaluator);
        xb_ha_secret_a.mod_switch_to_next_inplace(bfv_parm->evaluator);
        // Bob send [x_add*ha*gb]_a} to alice;

        // Alice: alice receive message and get x * gb;
        BFVLongPlaintext xgb_ha_plain = xb_ha_secret_a.decrypt(alice);
        bfv_matrix x_gb_ha_matrix = xgb_ha_plain.decode(bfv_parm); // something wrong here

        FixArray fix_x_gb(sci::BOB, batch_size * d_module, true, DEFAULT_ELL, DEFAULT_SCALE);
        uint64_t *x_gb_ha_prime = new uint64_t[batch_size * d_module];
        uint64_t *x_gb_ha_ring = new uint64_t[batch_size * d_module];

        for (size_t i = 0; i < batch_size * d_module; i++)
        {
            x_gb_ha_prime[i] = x_gb_ha_matrix[i];
        }
        conv->Prime_to_Ring(x_gb_ha_prime, x_gb_ha_ring, batch_size * d_module, DEFAULT_ELL, bfv_parm->plain_mod, DEFAULT_SCALE, DEFAULT_SCALE, fpmath_public);

        fix_x_gb = fpmath_bob->fix->input(sci::BOB, batch_size * d_module, x_gb_ha_ring, true, DEFAULT_ELL, DEFAULT_SCALE * 2);

        fix_x_gb = fpmath_alice->fix->location_truncation(fix_x_gb, DEFAULT_SCALE);

        fix_div_ha.party = sci::PUBLIC;
        fix_x_gb = fpmath_bob->fix->mul(fix_x_gb, fix_div_ha, DEFAULT_ELL);
        fix_x_gb = fpmath_bob->fix->location_truncation(fix_x_gb, DEFAULT_SCALE);

        vector<FixArray> vec_x_gb;

        for (size_t i = 0; i < batch_size; i++)
        {
            vec_x_gb.push_back(fpmath_bob->fix->input(fix_x_gb.party, d_module, &fix_x_gb.data[i * d_module], fix_x_gb.signed_, fix_x_gb.ell, fix_x_gb.s));
        }

        vector<FixArray> fix_mean_g = fpmath_bob->mean(vec_x_gb); // dim:  batch_size * 1

        vector<FixArray> delta_gb = fpmath_bob->standard_deviation(vec_x_gb, fix_mean_g); // delta -1/2

        double ka = dist(gen);
        FixArray fix_ka = fpmath_alice->fix->input(sci::ALICE, 1,
                                                   (sci::neg_mod(static_cast<int64_t>(ka * (1ULL << (DEFAULT_SCALE))), DEFAULT_ELL)),
                                                   true, DEFAULT_ELL, DEFAULT_SCALE);
        FixArray fix_div_ka = fpmath_alice->fix->input(sci::ALICE, 1,
                                                       (sci::neg_mod(static_cast<int64_t>((1.0 / ka) * (1ULL << (DEFAULT_SCALE))), DEFAULT_ELL)),
                                                       true, DEFAULT_ELL, DEFAULT_SCALE);

        vector<FixArray> ir_tmp1(batch_size);
        uint64_t *tmp1 = new uint64_t[batch_size * d_module];
        uint64_t *tmp2 = new uint64_t[batch_size * d_module];
        for (size_t i = 0; i < batch_size; i++)
        {
            vec_x_gb[i].party = sci::PUBLIC;
            ir_tmp1[i] = fpmath_public->fix->sub(vec_x_gb[i], fix_mean_g[i].data[0]);
            ir_tmp1[i] = fpmath_alice->fix->mul(ir_tmp1[i], fix_ka.data[0], DEFAULT_ELL);
            ir_tmp1[i] = fpmath_alice->fix->location_truncation(ir_tmp1[i], DEFAULT_SCALE);
            for (size_t j = 0; j < d_module; j++)
            {
                tmp1[i * d_module + j] = ir_tmp1[i].data[j];
                tmp2[i * d_module + j] = delta_gb[i].data[j];
            }
        }
        FixArray ret_tmp1 = fpmath_alice->fix->input(sci::ALICE, batch_size * d_module, tmp1, true, DEFAULT_ELL, DEFAULT_SCALE);
        // tmp * gama
        bfv_matrix gamma(batch_size * d_module);
        bfv_matrix beta(batch_size * d_module);
        random_bfv_mat(gamma);
        random_modP_mat(beta, bfv_parm->plain_mod);
        uint64_t *gama_array = new uint64_t[batch_size * d_module];
        uint64_t *beta_array = new uint64_t[batch_size * d_module];
        for (size_t i = 0; i < batch_size * d_module; i++)
        {
            gama_array[i] = gamma[i];
            beta_array[i] = beta[i];
        }
        FixArray gama_fix = fpmath_bob->fix->input(sci::BOB, batch_size * d_module, gama_array, true, DEFAULT_ELL, DEFAULT_SCALE);
        FixArray gama_tmp1(sci::BOB, batch_size * d_module, true, DEFAULT_ELL, DEFAULT_SCALE);

        ret_tmp1.party = sci::PUBLIC;
        gama_tmp1 = fpmath_bob->fix->mul(ret_tmp1, gama_fix, DEFAULT_ELL);
        gama_tmp1 = fpmath_bob->fix->location_truncation(gama_tmp1, DEFAULT_SCALE);
        conv->Ring_to_Prime(tmp1, tmp1, batch_size * d_module, DEFAULT_ELL, bfv_parm->plain_mod);
        conv->Ring_to_Prime(tmp2, tmp2, batch_size * d_module, DEFAULT_ELL, bfv_parm->plain_mod);

        conv->Ring_to_Prime(fix_div_ha.data[0], fix_div_ka.data[0], DEFAULT_ELL, bfv_parm->plain_mod);
        BFVLongPlaintext tmp1_plain(bfv_parm, tmp1, batch_size * d_module);
        BFVLongCiphertext layernorm_secret_a(bfv_parm, fix_div_ha.data[0], alice); // somthing wrong here;
        layernorm_secret_a.multiply_plain_inplace(tmp1_plain, bfv_parm->evaluator);
        layernorm_secret_a.mod_switch_to_next_inplace(bfv_parm->evaluator);
        BFVLongPlaintext beta_plain(bfv_parm, beta_array, batch_size * d_module);
        layernorm_secret_a.add_plain_inplace(beta_plain, bfv_parm->evaluator);

        // BFVLongPlaintext result = layernorm_secret_a.decrypt(alice);
        // bfv_matrix result_matrix = result.decode(bfv_parm); // something wrong here

        // std::cout << "Secure LayerNorm1 done.\n";
        // for (size_t i = 0; i < batch_size * d_module; i++)
        // {
        //     std::cout << result_matrix[i] << " ";
        // }

        delete[] prime_xb;
        delete[] prime_ha;
        delete[] prime_ha_xa;
        delete[] prime_gb;
        delete[] x_gb_ha_ring;
        delete[] x_gb_ha_prime;
        delete[] gama_array;
        delete[] beta_array;
        delete[] tmp1;
        delete[] tmp2;
    }
};

int main()
{

    uint64_t mask = (DEFAULT_ELL == 64 ? -1 : ((1ULL << DEFAULT_ELL) - 1));

    BFVParm *bfv_parm = new BFVParm(8192, {54, 54, 55, 55}, default_prime_mod.at(29));

    BFVKey *alice = new BFVKey(sci::ALICE, bfv_parm);
    BFVKey *bob = new BFVKey(sci::BOB, bfv_parm);

    matrix attn(batch_size * d_module),
        input_a(batch_size * d_module), input_b(batch_size * d_module);

    random_mat(attn, -1, 1, false);
    random_mat(input_a, -1, 1, false);
    random_mat(input_b, -1, 1, false);

    int64_t *int_attn = new int64_t[batch_size * d_module];
    uint64_t *uint_attn = new uint64_t[batch_size * d_module];

    int64_t *int_input_a = new int64_t[batch_size * d_module];
    uint64_t *uint_input_a = new uint64_t[batch_size * d_module];

    int64_t *int_input_b = new int64_t[batch_size * d_module];
    uint64_t *uint_input_b = new uint64_t[batch_size * d_module];

    // data prepare!
    for (size_t i = 0; i < batch_size * d_module; i++)
    {
        int_attn[i] = static_cast<int64_t>(attn[i] * (1ULL << (DEFAULT_SCALE)));
        uint_attn[i] = sci::neg_mod(int_attn[i], (int64_t)(1ULL << DEFAULT_ELL));

        int_input_a[i] = static_cast<int64_t>(input_a[i] * (1ULL << DEFAULT_SCALE));
        uint_input_a[i] = sci::neg_mod(int_input_a[i], (int64_t)(1ULL << DEFAULT_ELL));

        int_input_b[i] = static_cast<int64_t>(input_b[i] * (1ULL << DEFAULT_SCALE));
        uint_input_b[i] = sci::neg_mod(int_input_b[i], (int64_t)(1ULL << DEFAULT_ELL));
        // std::cout << input_a[i] << " ";
    }

    BFVLongPlaintext attn_plain(bfv_parm, uint_attn, batch_size * d_module);
    BFVLongCiphertext attn_secret_s(attn_plain, bob);
    size_t i, j;
    sci::IOPack *iopack;
    sci::OTPack *otpack;
    SecureLayerNorm1 *sec_ln1 = new SecureLayerNorm1(bfv_parm, alice, bob, iopack, otpack);
    INIT_TIMER
    START_TIMER
    sec_ln1->forward(attn_secret_s, uint_input_a, uint_input_b);
    STOP_TIMER("Secure LayerNorm")
    delete sec_ln1;
    delete alice;
    delete bob;
    delete bfv_parm;
    delete[] int_attn;
    delete[] uint_attn;
    delete[] int_input_a;
    delete[] uint_input_a;
    delete[] int_input_b;
    delete[] uint_input_b;
}
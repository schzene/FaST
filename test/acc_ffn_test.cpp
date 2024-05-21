#include <model.h>
#define TEST

#define NEG_SQRT_8_DIV_PI -1.59576912160573081145287233084673061966896057128906

typedef double (*activate_function)(double);

// inline double gelu(double x) {
//     return x / (1 + exp(NEG_SQRT_8_DIV_PI * (x + 0.047715 * x * x * x)));
// }

inline double gelu(double x) {
    double x2 = x * x, x3 = x2 * x, x4 = x2 * x2;
    if (x < -5.072) {
        return 0;
    }
    if (x < -1.414) {
        return -0.568686678 - 0.529288810 * x - 0.183509590 * x2 - 0.028070202 * x3 - 0.001597741 * x4;
    }
    if (x < 1.414) {
        return 0.001193207 + 0.5 * x + 0.385858026 * x2 - 0.045101361 * x4;
    }
    if (x < 5.072) {
        return -0.438406187 + 1.340789252 * x - 0.087184212 * x2 + 0.007334718 * x3;
    }
    return x;
}

class SecureFFN {
    CKKSKey *alice;
    CKKSKey *bob;
    CKKSEncoder *encoder;
    Evaluator *evaluator;

    inline void activate(matrix &mat, activate_function activate_func) {
        size_t size = mat.size();
        for (size_t i = 0; i < size; i++) {
            mat[i] = activate_func(mat[i]);
        }
    }

    LongCiphertext f3(const LongCiphertext &x1, const LongCiphertext &x2, const LongCiphertext &x3) {
        LongPlaintext parm0(-0.438406187, encoder), parm1(1.340789252, encoder), parm2(-0.087184212, encoder), parm3(0.007334718, encoder);
        parm3.mod_switch_to_inplace(x3.parms_id(), evaluator);
        parm2.mod_switch_to_inplace(x2.parms_id(), evaluator);
        parm1.mod_switch_to_inplace(x1.parms_id(), evaluator);
        LongCiphertext f3_ = x3.multiply_plain(parm3, evaluator), tmp1 = x2.multiply_plain(parm2, evaluator), tmp2 = x1.multiply_plain(parm1, evaluator);
        f3_.add_inplace(tmp1, evaluator);
        f3_.add_inplace(tmp2, evaluator);
        parm0.mod_switch_to_inplace(f3_.parms_id(), evaluator);
        f3_.add_plain_inplace(parm0, evaluator);
        return f3_;
    }

    LongCiphertext f2(const LongCiphertext &x1, const LongCiphertext &x2, const LongCiphertext &x4) {
        LongPlaintext parm0(0.001193207, encoder), parm1(0.5, encoder), parm2(0.385858026, encoder), parm4(-0.045101361, encoder);
        parm4.mod_switch_to_inplace(x4.parms_id(), evaluator);
        parm2.mod_switch_to_inplace(x2.parms_id(), evaluator);
        parm1.mod_switch_to_inplace(x1.parms_id(), evaluator);
        LongCiphertext f2_ = x4.multiply_plain(parm4, evaluator), tmp2 = x2.multiply_plain(parm2, evaluator), tmp3 = x1.multiply_plain(parm1, evaluator);
        f2_.add_inplace(tmp2, evaluator);
        f2_.add_inplace(tmp3, evaluator);
        parm0.mod_switch_to_inplace(f2_.parms_id(), evaluator);
        f2_.add_plain_inplace(parm0, evaluator);
        return f2_;
    }

    LongCiphertext f1(const LongCiphertext &x1, const LongCiphertext &x2, const LongCiphertext &x3, const LongCiphertext &x4) {
        LongPlaintext parm0(-0.568686678, encoder), parm1(-0.529288810, encoder), parm2(-0.183509590, encoder), parm3(-0.028070202, encoder), parm4(-0.001597741, encoder);
        parm4.mod_switch_to_inplace(x4.parms_id(), evaluator);
        parm3.mod_switch_to_inplace(x3.parms_id(), evaluator);
        parm2.mod_switch_to_inplace(x2.parms_id(), evaluator);
        parm1.mod_switch_to_inplace(x1.parms_id(), evaluator);
        LongCiphertext f1_ = x4.multiply_plain(parm3, evaluator), tmp1 = x3.multiply_plain(parm3, evaluator), tmp2 = x2.multiply_plain(parm2, evaluator), tmp3 = x1.multiply_plain(parm1, evaluator);
        f1_.add_inplace(tmp1, evaluator);
        f1_.add_inplace(tmp2, evaluator);
        f1_.add_inplace(tmp3, evaluator);
        parm0.mod_switch_to_inplace(f1_.parms_id(), evaluator);
        f1_.add_plain_inplace(parm0, evaluator);
        return f1_;
    }

public:
    SecureFFN(CKKSKey *alice_, CKKSKey *bob_,
              CKKSEncoder *encoder_, Evaluator *evaluator_) : alice(alice_),
                                                              bob(bob_),
                                                              encoder(encoder_),
                                                              evaluator(evaluator_) {}

    ~SecureFFN() {}

    void forward(const LongCiphertext &ln_secret_a) {
        size_t i, j;
        matrix W1a(d_module * ffn_dim), W1b(d_module * ffn_dim),
            B1a(batch_size * ffn_dim), B1b(batch_size * ffn_dim),
            W2a(ffn_dim * d_module), W2b(ffn_dim * d_module),
            B2a(batch_size * d_module), B2b(batch_size * d_module),
            W1(d_module * ffn_dim), B1(batch_size * ffn_dim), W2(ffn_dim * d_module), B2(batch_size * d_module),
            xb(batch_size * d_module), neg_xb(batch_size * d_module);
        random_mat(W1a);
        random_mat(W1b);
        random_mat(B1a);
        random_mat(B1b);
        random_mat(W2a);
        random_mat(W2b);
        random_mat(B2a);
        random_mat(B2b);
        random_mat(xb);

#ifdef TEST
        for (i = 0; i < ffn_dim * d_module; i++) {
            W1[i] = W1a[i] + W1b[i];
            W2[i] = W2a[i] + W2b[i];
        }
        for (i = 0; i < batch_size * ffn_dim; i++) {
            B1[i] = B1a[i] + B1b[i];
        }
        for (i = 0; i < batch_size * d_module; i++) {
            B2[i] = B2a[i] + B2b[i];
        }
#endif

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(-1, 1);

        for (i = 0; i < batch_size * d_module; i++) {
            neg_xb[i] = -xb[i];
        }
        LongPlaintext neg_xb_plain(neg_xb, encoder);
        LongCiphertext xa_secret_a = ln_secret_a.add_plain(neg_xb_plain, evaluator);
        // send xa_secret_a to alice

        LongPlaintext xa_plain = xa_secret_a.decrypt(alice);
        matrix va_xa = xa_plain.decode(encoder);
        double va = 1; // dist(gen);
        for (i = 0; i < batch_size * d_module; i++) {
            va_xa[i] *= va;
        }
        matrix va_xa_W1a = matmul(va_xa, W1a, batch_size, d_module, ffn_dim);
        for (i = 0; i < d_module * ffn_dim; i++) {
            W1a[i] *= va;
        }
        LongPlaintext B1a_plain(B1a, encoder), div_va_plain(1. / va, encoder);
        LongCiphertext B1a_secret_a(B1a_plain, alice), div_va_secret_a(div_va_plain, alice);
        // send va_xa, va_xa_W1a, W1a, b1a_secret_a, div_va_secret_a to bob

        matrix tmp1 = matmul(va_xa, W1b, batch_size, d_module, ffn_dim);
        matrix tmp2 = matmul(xb, W1a, batch_size, d_module, ffn_dim);
        matrix xb_W1b = matmul(xb, W1b, batch_size, d_module, ffn_dim);
        for (i = 0; i < batch_size * ffn_dim; i++) {
            tmp1[i] = tmp1[i] + tmp2[i] + va_xa_W1a[i];
        }
        LongPlaintext tmp1_plain(tmp1, encoder), xb_W1b_plain(xb_W1b, encoder), B1b_plain(B1b, encoder);
        LongCiphertext x1_secret_a = div_va_secret_a.multiply_plain(tmp1_plain, evaluator);
        xb_W1b_plain.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        x1_secret_a.add_plain_inplace(xb_W1b_plain, evaluator);
        B1a_secret_a.add_plain_inplace(B1b_plain, evaluator);
        B1a_secret_a.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        x1_secret_a.add_inplace(B1a_secret_a, evaluator);

        LongPlaintext parm0(5.075, encoder), parm1(sqrt(2), encoder), parm2(-sqrt(2), encoder), parm3(-5.075, encoder);
        parm0.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        parm1.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        parm2.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        parm3.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        LongCiphertext s0_sgn_secret_a = x1_secret_a.add_plain(parm0, evaluator),
                       s1_sgn_secret_a = x1_secret_a.add_plain(parm1, evaluator),
                       s2_sgn_secret_a = x1_secret_a.add_plain(parm2, evaluator),
                       s3_sgn_secret_a = x1_secret_a.add_plain(parm3, evaluator);
        LongPlaintext s3_plain = s3_sgn_secret_a.decrypt(alice);
        matrix s0_sgn_b(batch_size * ffn_dim), s1_sgn_b(batch_size * ffn_dim), s2_sgn_b(batch_size * ffn_dim), s3_sgn_b(batch_size * ffn_dim);
        random_mat(s0_sgn_b, -1, 1, true);
        random_mat(s1_sgn_b, -1, 1, true);
        random_mat(s2_sgn_b, -1, 1, true);
        random_mat(s3_sgn_b, -1, 1, true);
        LongPlaintext s0_sgn_plain_b(s0_sgn_b, encoder), s1_sgn_plain_b(s1_sgn_b, encoder), s2_sgn_plain_b(s2_sgn_b, encoder), s3_sgn_plain_b(s3_sgn_b, encoder);
        s0_sgn_plain_b.mod_switch_to_inplace(s0_sgn_secret_a.parms_id(), evaluator);
        s1_sgn_plain_b.mod_switch_to_inplace(s1_sgn_secret_a.parms_id(), evaluator);
        s2_sgn_plain_b.mod_switch_to_inplace(s2_sgn_secret_a.parms_id(), evaluator);
        s3_sgn_plain_b.mod_switch_to_inplace(s3_sgn_secret_a.parms_id(), evaluator);
        s0_sgn_secret_a.multiply_plain_inplace(s0_sgn_plain_b, evaluator);
        s1_sgn_secret_a.multiply_plain_inplace(s1_sgn_plain_b, evaluator);
        s2_sgn_secret_a.multiply_plain_inplace(s2_sgn_plain_b, evaluator);
        s3_sgn_secret_a.multiply_plain_inplace(s3_sgn_plain_b, evaluator);

        double rs = dist(gen);
        LongPlaintext rs_plain(rs, encoder);
        rs_plain.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        x1_secret_a.multiply_plain_inplace(rs_plain, evaluator);
        // send s0_sgn_secret_a, s1_sgn_secret_a, s2_sgn_secret_a, s3_sgn_secret_a, x1_secret_a to alice

        LongPlaintext rs_x1_plain = x1_secret_a.decrypt(alice);
        matrix rs_rc_x1 = rs_x1_plain.decode(encoder);
        double rc = dist(gen);
        LongPlaintext s0_sgn_plain_a = s0_sgn_secret_a.decrypt(alice),
                      s1_sgn_plain_a = s1_sgn_secret_a.decrypt(alice),
                      s2_sgn_plain_a = s2_sgn_secret_a.decrypt(alice),
                      s3_sgn_plain_a = s3_sgn_secret_a.decrypt(alice);
        matrix s0_sgn_a = s0_sgn_plain_a.decode(encoder),
               s1_sgn_a = s1_sgn_plain_a.decode(encoder),
               s2_sgn_a = s2_sgn_plain_a.decode(encoder),
               s3_sgn_a = s3_sgn_plain_a.decode(encoder);
        for (i = 0; i < batch_size * ffn_dim; i++) {
            s0_sgn_a[i] = s0_sgn_a[i] > 0 ? 1 : -1;
            s1_sgn_a[i] = s1_sgn_a[i] > 0 ? 1 : -1;
            s2_sgn_a[i] = s2_sgn_a[i] > 0 ? 1 : -1;
            s3_sgn_a[i] = s3_sgn_a[i] > 0 ? 1 : -1;
            rs_rc_x1[i] *= rc;
        }
        double div_rc = 1. / rc, div_rc_2 = div_rc * div_rc, div_rc_3 = div_rc_2 * div_rc, div_rc_4 = div_rc_2 * div_rc_2;
        LongPlaintext div_rc_plain(div_rc, encoder),
            div_rc_2_plain(div_rc_2, encoder),
            div_rc_3_plain(div_rc_3, encoder),
            div_rc_4_plain(div_rc_4, encoder);
        LongCiphertext x1_secret_a_(div_rc_plain, alice),
            x1_2_secret_a_(div_rc_2_plain, alice),
            x1_3_secret_a_(div_rc_3_plain, alice),
            x1_4_secret_a_(div_rc_4_plain, alice);
        // send s0_sgn_a, s1_sgn_a, s2_sgn_a, s3_sgn_a, rs_rc_x1, x1_secret_a_, x1_2_secret_a_, x1_3_secret_a_, x1_4_secret_a_ to bob;

        for (i = 0; i < batch_size * ffn_dim; i++) {
            s0_sgn_b[i] = s0_sgn_a[i] * s0_sgn_b[i] > 0 ? 1 : 0;
            s1_sgn_b[i] = s1_sgn_a[i] * s1_sgn_b[i] > 0 ? 1 : 0;
            s2_sgn_b[i] = s2_sgn_a[i] * s2_sgn_b[i] > 0 ? 1 : 0;
            s3_sgn_b[i] = s3_sgn_a[i] * s3_sgn_b[i] > 0 ? 1 : 0;
            rs_rc_x1[i] /= rs;
        }
        matrix b1(batch_size * ffn_dim), b2(batch_size * ffn_dim), b3(batch_size * ffn_dim), b4(batch_size * ffn_dim);
        matrix x1_2(batch_size * ffn_dim), x1_3(batch_size * ffn_dim), x1_4(batch_size * ffn_dim);
        for (i = 0; i < batch_size * ffn_dim; i++) {
            b1[i] = (s0_sgn_b[i] == 1 && s1_sgn_b[i] == 0) ? 1 : 1e-9;
            b2[i] = (s1_sgn_b[i] == 1 && s2_sgn_b[i] == 0) ? 1 : 1e-9;
            b3[i] = (s2_sgn_b[i] == 1 && s3_sgn_b[i] == 0) ? 1 : 1e-9;
            b4[i] = s3_sgn_b[i] == 1 ? 1 : 1e-9;
            x1_2[i] = rs_rc_x1[i] * rs_rc_x1[i];
            x1_3[i] = x1_2[i] * rs_rc_x1[i];
            x1_4[i] = x1_2[i] * x1_2[i];
        }
        LongPlaintext x1_plain(rs_rc_x1, encoder), x1_2_plain(x1_2, encoder), x1_3_plain(x1_3, encoder), x1_4_plain(x1_4, encoder);
        x1_secret_a_.multiply_plain_inplace(x1_plain, evaluator);
        x1_2_secret_a_.multiply_plain_inplace(x1_2_plain, evaluator);
        x1_3_secret_a_.multiply_plain_inplace(x1_3_plain, evaluator);
        x1_4_secret_a_.multiply_plain_inplace(x1_4_plain, evaluator);
        LongPlaintext b1_plain(b1, encoder), b2_plain(b2, encoder), b3_plain(b3, encoder), b4_plain(b4, encoder);

        b4_plain.mod_switch_to_inplace(x1_secret_a_.parms_id(), evaluator);
        LongCiphertext gelu_secret_a = x1_secret_a_.multiply_plain(b4_plain, evaluator);

        LongCiphertext f3_ = f3(x1_secret_a_, x1_2_secret_a_, x1_3_secret_a_);
        // b3_plain.mod_switch_to_inplace(f3_.parms_id(), evaluator);
        // f3_.multiply_plain_inplace(b3_plain, evaluator);
        // LongCiphertext f2_ = f2(x1_secret_a_, x1_2_secret_a_, x1_4_secret_a_);
        // b2_plain.mod_switch_to_inplace(f2_.parms_id(), evaluator);
        // f2_.multiply_plain_inplace(b2_plain, evaluator);
        // LongCiphertext f1_ = f1(x1_secret_a_, x1_2_secret_a_, x1_3_secret_a_, x1_4_secret_a_);
        // b1_plain.mod_switch_to_inplace(f1_.parms_id(), evaluator);
        // f1_.multiply_plain_inplace(b1_plain, evaluator);
        // gelu_secret_a.add_inplace(f3_, evaluator);
        // gelu_secret_a.add_inplace(f2_, evaluator);
        // gelu_secret_a.add_inplace(f1_, evaluator);

        LongPlaintext gelu_plain = gelu_secret_a.decrypt(alice);
        matrix gelu_ = gelu_plain.decode(encoder);
        print_mat(gelu_, batch_size, ffn_dim);

        std::cout << "Secure Feed Forward done.\n";

#ifdef TEST
        auto ln_plain = ln_secret_a.decrypt(alice);
        auto ln = ln_plain.decode(encoder);
        auto x1 = matmul(ln, W1, batch_size, d_module, ffn_dim);
        for (i = 0; i < batch_size * ffn_dim; i++) {
            x1[i] += B1[i];
        }
        activate(x1, gelu);
        print_mat(x1, batch_size, ffn_dim);
        auto x2 = matmul(x1, W2, batch_size, ffn_dim, d_module);
        for (i = 0; i < batch_size * d_module; i++) {
            x2[i] += B2[i];
        }
        // print_mat(x2, batch_size, d_module);
#endif
    }
};

int main() {
    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    SEALContext *context = new SEALContext(parms);
    CKKSEncoder *encoder = new CKKSEncoder(*context);
    Evaluator *evaluator = new Evaluator(*context);

    CKKSKey *alice = new CKKSKey(1, context);
    CKKSKey *bob = new CKKSKey(2, context);
    matrix input(batch_size * d_module);
    random_mat(input, 0, 0.01);
    LongPlaintext input_plain(input, encoder);
    LongCiphertext input_secret_a(input_plain, alice);
    SecureFFN *ffn = new SecureFFN(alice, bob, encoder, evaluator);
    ffn->forward(input_secret_a);

    delete context;
    delete encoder;
    delete evaluator;
    delete alice;
    delete bob;
    delete ffn;
}
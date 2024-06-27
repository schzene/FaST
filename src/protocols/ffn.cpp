#include "ffn.h"

LongCiphertext FFN::f3(const LongCiphertext &x1, const LongCiphertext &x2,
                       const LongCiphertext &x3) {
    LongPlaintext parm0(-0.438406187, encoder), parm1(1.340789252, encoder),
        parm2(-0.087184212, encoder), parm3(0.007334718, encoder);
    parm3.mod_switch_to_inplace(x3.parms_id(), evaluator);
    parm2.mod_switch_to_inplace(x2.parms_id(), evaluator);
    parm1.mod_switch_to_inplace(x1.parms_id(), evaluator);
    LongCiphertext f3_ = x3.multiply_plain(parm3, evaluator),
                   tmp1 = x2.multiply_plain(parm2, evaluator),
                   tmp2 = x1.multiply_plain(parm1, evaluator);
    f3_.add_inplace(tmp1, evaluator);
    f3_.add_inplace(tmp2, evaluator);
    parm0.mod_switch_to_inplace(f3_.parms_id(), evaluator);
    f3_.add_plain_inplace(parm0, evaluator);
    return f3_;
}

LongCiphertext FFN::f2(const LongCiphertext &x1, const LongCiphertext &x2,
                       const LongCiphertext &x4) {
    LongPlaintext parm0(0.001193207, encoder), parm1(0.5, encoder),
        parm2(0.385858026, encoder), parm4(-0.045101361, encoder);
    parm4.mod_switch_to_inplace(x4.parms_id(), evaluator);
    parm2.mod_switch_to_inplace(x2.parms_id(), evaluator);
    parm1.mod_switch_to_inplace(x1.parms_id(), evaluator);
    LongCiphertext f2_ = x4.multiply_plain(parm4, evaluator),
                   tmp2 = x2.multiply_plain(parm2, evaluator),
                   tmp3 = x1.multiply_plain(parm1, evaluator);
    f2_.add_inplace(tmp2, evaluator);
    f2_.add_inplace(tmp3, evaluator);
    parm0.mod_switch_to_inplace(f2_.parms_id(), evaluator);
    f2_.add_plain_inplace(parm0, evaluator);
    return f2_;
}

LongCiphertext FFN::f1(const LongCiphertext &x1, const LongCiphertext &x2,
                       const LongCiphertext &x3, const LongCiphertext &x4) {
    LongPlaintext parm0(-0.568686678, encoder), parm1(-0.529288810, encoder),
        parm2(-0.183509590, encoder), parm3(-0.028070202, encoder),
        parm4(-0.001597741, encoder);
    parm4.mod_switch_to_inplace(x4.parms_id(), evaluator);
    parm3.mod_switch_to_inplace(x3.parms_id(), evaluator);
    parm2.mod_switch_to_inplace(x2.parms_id(), evaluator);
    parm1.mod_switch_to_inplace(x1.parms_id(), evaluator);
    LongCiphertext f1_ = x4.multiply_plain(parm3, evaluator),
                   tmp1 = x3.multiply_plain(parm3, evaluator),
                   tmp2 = x2.multiply_plain(parm2, evaluator),
                   tmp3 = x1.multiply_plain(parm1, evaluator);
    f1_.add_inplace(tmp1, evaluator);
    f1_.add_inplace(tmp2, evaluator);
    f1_.add_inplace(tmp3, evaluator);
    parm0.mod_switch_to_inplace(f1_.parms_id(), evaluator);
    f1_.add_plain_inplace(parm0, evaluator);
    return f1_;
}

LongCiphertext FFN::forward(const LongCiphertext &ln1) {
    size_t i, j;
    size_t total_comm = 0;
    matrix W1(d_module * ffn_dim), B1(batch_size * ffn_dim),
        W2(ffn_dim * d_module), B2(batch_size * d_module);
    load_mat(W1, "WQa-${party}");
    load_mat(B1, "WKa-${party}");
    load_mat(W2, "WQa-${party}");
    load_mat(B2, "WKa-${party}");
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-1, 1);
    if (party->party == sci::ALICE) {
        double va = dist(gen), rc = dist(gen), v1a = dist(gen);
        double div_rc = 1. / rc, div_rc_2 = div_rc * div_rc,
               div_rc_3 = div_rc_2 * div_rc, div_rc_4 = div_rc_2 * div_rc_2;
        LongPlaintext B1_plain(B1, encoder), div_va_plain(1. / va, encoder),
            div_rc_plain(div_rc, encoder), div_rc_2_plain(div_rc_2, encoder),
            div_rc_3_plain(div_rc_3, encoder),
            div_rc_4_plain(div_rc_4, encoder), B2a_plain(B2, encoder),
            div_v1a_plain(1. / v1a, encoder);
        LongCiphertext x_secret_a, B1_secret_a(B1_plain, party),
            div_va_secret_a(div_va_plain, party), s0_sgn_secret_a,
            s1_sgn_secret_a, s2_sgn_secret_a, s3_sgn_secret_a, x1_secret_a,
            div_rc_secret_a(div_rc_plain, party),
            div_rc_2_secret_a(div_rc_2_plain, party),
            div_rc_3_secret_a(div_rc_3_plain, party),
            div_rc_4_secret_a(div_rc_4_plain, party), f4_, f3_, f2_, f1_,
            gelu_secret_a, B2a_secret_a(B2a_plain, party),
            div_v1a_secret_a(div_v1a_plain, party), x2_secret_a, vb_secert_b;
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif
        // alice receive x_secret_a
        LongCiphertext::recv(io, &x_secret_a, party->context);
        LongPlaintext x_plain = x_secret_a.decrypt(party);
        matrix va_x = x_plain.decode(encoder);
        for (i = 0; i < batch_size * d_module; i++) {
            va_x[i] *= va;
        }
        matrix va_x_W1a = matmul(va_x, W1, batch_size, d_module, ffn_dim);
        for (i = 0; i < d_module * ffn_dim; i++) {
            W1[i] *= va;
        }

        // send va_x, va_x_W1a, W1, B1_secret_a, div_va_secret_a to bob
        send_mat(io, &va_x);
        send_mat(io, &va_x_W1a);
        send_mat(io, &W1);
        LongCiphertext::send(io, &B1_secret_a);
        LongCiphertext::send(io, &div_va_secret_a);

        // alice receive s0_sgn_secret_a, s1_sgn_secret_a, s2_sgn_secret_a,
        // s3_sgn_secret_a, x1_secret_a
        LongCiphertext::recv(io, &s0_sgn_secret_a, party->context);
        LongCiphertext::recv(io, &s1_sgn_secret_a, party->context);
        LongCiphertext::recv(io, &s2_sgn_secret_a, party->context);
        LongCiphertext::recv(io, &s3_sgn_secret_a, party->context);
        LongCiphertext::recv(io, &x1_secret_a, party->context);

        LongPlaintext rs_x1_plain = x1_secret_a.decrypt(party);
        matrix rs_rc_x1 = rs_x1_plain.decode(encoder);
        LongPlaintext s0_sgn_plain_a = s0_sgn_secret_a.decrypt(party),
                      s1_sgn_plain_a = s1_sgn_secret_a.decrypt(party),
                      s2_sgn_plain_a = s2_sgn_secret_a.decrypt(party),
                      s3_sgn_plain_a = s3_sgn_secret_a.decrypt(party);
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
        // send s0_sgn_a, s1_sgn_a, s2_sgn_a, s3_sgn_a, rs_rc_x1,
        // div_rc_secret_a, div_rc_2_secret_a, div_rc_3_secret_a,
        // div_rc_4_secret_a to bob;
        send_mat(io, &s0_sgn_a);
        send_mat(io, &s1_sgn_a);
        send_mat(io, &s2_sgn_a);
        send_mat(io, &s3_sgn_a);
        send_mat(io, &rs_rc_x1);
        LongCiphertext::send(io, &div_rc_secret_a);
        LongCiphertext::send(io, &div_rc_2_secret_a);
        LongCiphertext::send(io, &div_rc_3_secret_a);
        LongCiphertext::send(io, &div_rc_4_secret_a);
#ifdef LOG
        PAUSE_TIMER("TODO1", false)
        /** TODO1
         * This is a narrow impl, as CKKS can not multiply too many times, so we
         * let alice decrypt the ciphertext and re-encrypt the data. We will
         * solve this problem later.
         */
        // alice recv f4_, f3_, f2_, f1_
        LongCiphertext::recv(io, &f4_, party->context, false);
        LongCiphertext::recv(io, &f3_, party->context, false);
        LongCiphertext::recv(io, &f2_, party->context, false);
        LongCiphertext::recv(io, &f1_, party->context, false);
        LongPlaintext f4_plain = f4_.decrypt(party),
                      f3_plain = f3_.decrypt(party),
                      f2_plain = f2_.decrypt(party),
                      f1_plain = f1_.decrypt(party);
        matrix f4_m = f4_plain.decode(encoder), f3_m = f3_plain.decode(encoder),
               f2_m = f2_plain.decode(encoder), f1_m = f1_plain.decode(encoder);
        LongPlaintext _f4_plain(f4_m, encoder), _f3_plain(f3_m, encoder),
            _f2_plain(f2_m, encoder), _f1_plain(f1_m, encoder);
        LongCiphertext _f4_(_f4_plain, party), _f3_(_f3_plain, party),
            _f2_(_f2_plain, party), _f1_(_f1_plain, party);
        // send _f3_, _f2_, _f1_ to bob
        LongCiphertext::send(io, &_f4_, false);
        LongCiphertext::send(io, &_f3_, false);
        LongCiphertext::send(io, &_f2_, false);
        LongCiphertext::send(io, &_f1_, false);
        /**
         * end TODO1
         */
        START_TIMER
#endif
        LongCiphertext::recv(io, &gelu_secret_a, party->context);
        LongPlaintext x1a_plain = gelu_secret_a.decrypt(party);
        matrix v1a_x1a = x1a_plain.decode(encoder);

        for (i = 0; i < batch_size * ffn_dim; i++) {
            v1a_x1a[i] *= v1a;
        }
        matrix v1a_x1a_W2a = matmul(v1a_x1a, W2, batch_size, ffn_dim, d_module);
        for (i = 0; i < ffn_dim * d_module; i++) {
            W2[i] *= v1a;
        }
        // send v1a_x1a, v1a_x1a_W2a, W2, B2a_secret_a, div_v1a_secret_a to bob
        send_mat(io, &v1a_x1a);
        send_mat(io, &v1a_x1a_W2a);
        send_mat(io, &W2);
        LongCiphertext::send(io, &B2a_secret_a);
        LongCiphertext::send(io, &div_v1a_secret_a);

        // alice receive x2_secert_a, vb_secret_b;
        LongCiphertext::recv(io, &x2_secret_a, party->context);
        LongCiphertext::recv(io, &vb_secert_b, party->context);
        LongPlaintext x2_plain_ = x2_secret_a.decrypt(party);
        matrix x2 = x2_plain_.decode(encoder);
        LongPlaintext x2_plain(x2, encoder);
        vb_secert_b.add_plain_inplace(x2_plain, evaluator);
#ifdef LOG
        STOP_TIMER("Feed Forward")
        total_comm += io->counter;
        std::cout << "Feed Forward Send data " << total_comm << " Bytes. \n";
#endif
        return vb_secert_b;
    } else {
        double rs = dist(gen), vb = dist(gen);
        matrix x(batch_size * d_module), neg_x(batch_size * d_module),
            x1(batch_size * ffn_dim), neg_x1(batch_size * ffn_dim);
        random_mat(x);
        random_mat(x1);
        for (i = 0; i < batch_size * d_module; i++) {
            neg_x[i] = -x[i];
        }
        for (i = 0; i < batch_size * ffn_dim; i++) {
            neg_x1[i] = -x1[i];
        }
        matrix s0_sgn_a(batch_size * ffn_dim), s1_sgn_a(batch_size * ffn_dim),
            s2_sgn_a(batch_size * ffn_dim), s3_sgn_a(batch_size * ffn_dim),
            va_xa(batch_size * d_module), va_xa_W1a(batch_size * ffn_dim),
            W1a(d_module * ffn_dim), s0_sgn_b(batch_size * ffn_dim),
            s1_sgn_b(batch_size * ffn_dim), s2_sgn_b(batch_size * ffn_dim),
            s3_sgn_b(batch_size * ffn_dim), b1(batch_size * ffn_dim),
            b2(batch_size * ffn_dim), b3(batch_size * ffn_dim),
            b4(batch_size * ffn_dim), x1_2(batch_size * ffn_dim),
            x1_3(batch_size * ffn_dim), x1_4(batch_size * ffn_dim),
            rs_rc_x1(batch_size * ffn_dim), v1a_x1a(batch_size * ffn_dim),
            v1a_x1a_W2a(batch_size * d_module), W2a(ffn_dim * d_module);
        random_mat(s0_sgn_b, -1, 1, true);
        random_mat(s1_sgn_b, -1, 1, true);
        random_mat(s2_sgn_b, -1, 1, true);
        random_mat(s3_sgn_b, -1, 1, true);
        LongPlaintext neg_xb_plain(neg_x, encoder), parm0(5.075, encoder),
            parm1(sqrt(2), encoder), parm2(-sqrt(2), encoder),
            parm3(-5.075, encoder), s0_sgn_plain_b(s0_sgn_b, encoder),
            s1_sgn_plain_b(s1_sgn_b, encoder),
            s2_sgn_plain_b(s2_sgn_b, encoder),
            s3_sgn_plain_b(s3_sgn_b, encoder), rs_plain(rs, encoder),
            neg_x1b_plain(neg_x1, encoder), div_vb_plain(-vb, encoder);
        LongCiphertext B1a_secret_a, div_va_secret_a, x1_secret_a_,
            x1_2_secret_a_, x1_3_secret_a_, x1_4_secret_a_, _f1_, _f2_, _f3_,
            _f4_, B2a_secret_a, div_v1a_secret_a,
            vb_secret_b(vb, party, encoder);
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif
        neg_xb_plain.mod_switch_to_inplace(ln1.parms_id(), evaluator);
        LongCiphertext xa_secret_a = ln1.add_plain(neg_xb_plain, evaluator);
        // send xa_secret_a to alice
        LongCiphertext::send(io, &xa_secret_a);

        // bob receive va_xa, va_xa_W1a, W1a, B1_secret_a, div_va_secret_a;
        recv_mat(io, &va_xa);
        recv_mat(io, &va_xa_W1a);
        recv_mat(io, &W1a);
        LongCiphertext::recv(io, &B1a_secret_a, party->context);
        LongCiphertext::recv(io, &div_va_secret_a, party->context);

        matrix tmp11 = matmul(va_xa, W1, batch_size, d_module, ffn_dim);
        matrix tmp12 = matmul(x, W1a, batch_size, d_module, ffn_dim);
        matrix xb_W1b = matmul(x, W1, batch_size, d_module, ffn_dim);
        for (i = 0; i < batch_size * ffn_dim; i++) {
            tmp11[i] = tmp11[i] + tmp12[i] + va_xa_W1a[i];
        }
        LongPlaintext tmp11_plain(tmp11, encoder),
            xb_W1b_plain(xb_W1b, encoder), B1b_plain(B1, encoder);
        LongCiphertext x1_secret_a =
            div_va_secret_a.multiply_plain(tmp11_plain, evaluator);
        xb_W1b_plain.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        x1_secret_a.add_plain_inplace(xb_W1b_plain, evaluator);
        B1a_secret_a.add_plain_inplace(B1b_plain, evaluator);
        B1a_secret_a.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        x1_secret_a.add_inplace(B1a_secret_a, evaluator);

        parm0.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        parm1.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        parm2.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        parm3.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        LongCiphertext s0_sgn_secret_a =
                           x1_secret_a.add_plain(parm0, evaluator),
                       s1_sgn_secret_a =
                           x1_secret_a.add_plain(parm1, evaluator),
                       s2_sgn_secret_a =
                           x1_secret_a.add_plain(parm2, evaluator),
                       s3_sgn_secret_a =
                           x1_secret_a.add_plain(parm3, evaluator);
        s0_sgn_plain_b.mod_switch_to_inplace(s0_sgn_secret_a.parms_id(),
                                             evaluator);
        s1_sgn_plain_b.mod_switch_to_inplace(s1_sgn_secret_a.parms_id(),
                                             evaluator);
        s2_sgn_plain_b.mod_switch_to_inplace(s2_sgn_secret_a.parms_id(),
                                             evaluator);
        s3_sgn_plain_b.mod_switch_to_inplace(s3_sgn_secret_a.parms_id(),
                                             evaluator);
        s0_sgn_secret_a.multiply_plain_inplace(s0_sgn_plain_b, evaluator);
        s1_sgn_secret_a.multiply_plain_inplace(s1_sgn_plain_b, evaluator);
        s2_sgn_secret_a.multiply_plain_inplace(s2_sgn_plain_b, evaluator);
        s3_sgn_secret_a.multiply_plain_inplace(s3_sgn_plain_b, evaluator);

        rs_plain.mod_switch_to_inplace(x1_secret_a.parms_id(), evaluator);
        x1_secret_a.multiply_plain_inplace(rs_plain, evaluator);
        // send s0_sgn_secret_a, s1_sgn_secret_a, s2_sgn_secret_a,
        // s3_sgn_secret_a, x1_secret_a to alice
        LongCiphertext::send(io, &s0_sgn_secret_a);
        LongCiphertext::send(io, &s1_sgn_secret_a);
        LongCiphertext::send(io, &s2_sgn_secret_a);
        LongCiphertext::send(io, &s3_sgn_secret_a);
        LongCiphertext::send(io, &x1_secret_a);

        // Bob receive s0_sgn_a, s1_sgn_a, s2_sgn_a, s3_sgn_a, rs_rc_x1,
        recv_mat(io, &s0_sgn_a);
        recv_mat(io, &s1_sgn_a);
        recv_mat(io, &s2_sgn_a);
        recv_mat(io, &s3_sgn_a);
        recv_mat(io, &rs_rc_x1);
        LongCiphertext::recv(io, &x1_secret_a_, party->context);
        LongCiphertext::recv(io, &x1_2_secret_a_, party->context);
        LongCiphertext::recv(io, &x1_3_secret_a_, party->context);
        LongCiphertext::recv(io, &x1_4_secret_a_, party->context);
        for (i = 0; i < batch_size * ffn_dim; i++) {
            s0_sgn_b[i] = s0_sgn_a[i] * s0_sgn_b[i] > 0 ? 1 : 0;
            s1_sgn_b[i] = s1_sgn_a[i] * s1_sgn_b[i] > 0 ? 1 : 0;
            s2_sgn_b[i] = s2_sgn_a[i] * s2_sgn_b[i] > 0 ? 1 : 0;
            s3_sgn_b[i] = s3_sgn_a[i] * s3_sgn_b[i] > 0 ? 1 : 0;
            rs_rc_x1[i] /= rs;
        }

        for (i = 0; i < batch_size * ffn_dim; i++) {
            b1[i] = (s0_sgn_b[i] == 1 && s1_sgn_b[i] == 0) ? 1 : 1e-15;
            b2[i] = (s1_sgn_b[i] == 1 && s2_sgn_b[i] == 0) ? 1 : 1e-15;
            b3[i] = (s2_sgn_b[i] == 1 && s3_sgn_b[i] == 0) ? 1 : 1e-15;
            b4[i] = s3_sgn_b[i] == 1 ? 1 : 1e-9;
            x1_2[i] = rs_rc_x1[i] * rs_rc_x1[i];
            x1_3[i] = x1_2[i] * rs_rc_x1[i];
            x1_4[i] = x1_2[i] * x1_2[i];
        }
        LongPlaintext x1_plain(rs_rc_x1, encoder), x1_2_plain(x1_2, encoder),
            x1_3_plain(x1_3, encoder), x1_4_plain(x1_4, encoder);
        x1_secret_a_.multiply_plain_inplace(x1_plain, evaluator);
        x1_2_secret_a_.multiply_plain_inplace(x1_2_plain, evaluator);
        x1_3_secret_a_.multiply_plain_inplace(x1_3_plain, evaluator);
        x1_4_secret_a_.multiply_plain_inplace(x1_4_plain, evaluator);
        LongPlaintext b1_plain(b1, encoder), b2_plain(b2, encoder),
            b3_plain(b3, encoder), b4_plain(b4, encoder);

        b4_plain.mod_switch_to_inplace(x1_secret_a_.parms_id(), evaluator);
        LongCiphertext f4_ = x1_secret_a_.multiply_plain(b4_plain, evaluator),
                       f3_ = f3(x1_secret_a_, x1_2_secret_a_, x1_3_secret_a_),
                       f2_ = f2(x1_secret_a_, x1_2_secret_a_, x1_4_secret_a_),
                       f1_ = f1(x1_secret_a_, x1_2_secret_a_, x1_3_secret_a_,
                                x1_4_secret_a_);
#ifdef LOG
        PAUSE_TIMER("TODO1", false)
        /** TODO1
         * This is a narrow impl, as CKKS can not multiply too many times, so we
         * let alice decrypt the ciphertext and re-encrypt the data. We will
         * solve this problem later.
         */
        // send f4_, f3_, f2_, f1_ to alice
        LongCiphertext::send(io, &f4_, false);
        LongCiphertext::send(io, &f3_, false);
        LongCiphertext::send(io, &f2_, false);
        LongCiphertext::send(io, &f1_, false);

        // bob recv _f4_, _f3_, _f2_, _f1_
        LongCiphertext::recv(io, &_f4_, party->context);
        LongCiphertext::recv(io, &_f3_, party->context, false);
        LongCiphertext::recv(io, &_f2_, party->context, false);
        LongCiphertext::recv(io, &_f1_, party->context, false);
        /**
         * end TODO1
         */
        START_TIMER
#endif
        _f3_.multiply_plain_inplace(b3_plain, evaluator);
        _f2_.multiply_plain_inplace(b2_plain, evaluator);
        _f1_.multiply_plain_inplace(b1_plain, evaluator);
        _f1_.add_inplace(_f3_, evaluator);
        _f1_.add_inplace(_f2_, evaluator);
        _f4_.mod_switch_to_inplace(_f1_.parms_id(), evaluator);
        _f1_.add_inplace(_f4_, evaluator); // _f1_ is gelu

        neg_x1b_plain.mod_switch_to_inplace(_f1_.parms_id(), evaluator);
        _f1_.add_plain_inplace(neg_x1b_plain, evaluator);
        // send _f1_ to alice
        LongCiphertext::send(io, &_f1_);

        // bob receive v1a_x1a, v1a_x1a_W2a, W2, B2a_secret_a, div_v1a_secret_a
        recv_mat(io, &v1a_x1a);
        recv_mat(io, &v1a_x1a_W2a);
        recv_mat(io, &W2a);
        LongCiphertext::recv(io, &B2a_secret_a, party->context);
        LongCiphertext::recv(io, &div_v1a_secret_a, party->context);

        matrix tmp21 = matmul(v1a_x1a, W2a, batch_size, ffn_dim, d_module);
        matrix tmp22 = matmul(x1, W2, batch_size, ffn_dim, d_module);
        matrix x1b_W2b = matmul(x1, W2, batch_size, ffn_dim, d_module);
        for (i = 0; i < batch_size * d_module; i++) {
            tmp21[i] = tmp21[i] + tmp22[i] + v1a_x1a_W2a[i];
        }
        LongPlaintext tmp21_plain(tmp21, encoder),
            x1b_W2b_plain(x1b_W2b, encoder), B2b_plain(B2, encoder);
        LongCiphertext x2_secret_a =
            div_v1a_secret_a.multiply_plain(tmp21_plain, evaluator);
        x1b_W2b_plain.mod_switch_to_inplace(x2_secret_a.parms_id(), evaluator);
        x2_secret_a.add_plain_inplace(x1b_W2b_plain, evaluator);
        B2a_secret_a.add_plain_inplace(B2b_plain, evaluator);
        B2a_secret_a.mod_switch_to_inplace(x2_secret_a.parms_id(), evaluator);
        x2_secret_a.add_inplace(B2a_secret_a, evaluator);

        div_vb_plain.mod_switch_to_inplace(x2_secret_a.parms_id(), evaluator);
        x2_secret_a.add_plain_inplace(div_vb_plain, evaluator);
        // send x2_secret_a, vs_secret_b to bob
        LongCiphertext::send(io, &x2_secret_a);
        LongCiphertext::send(io, &vb_secret_b);
#ifdef LOG
        STOP_TIMER("Feed Forward")
        total_comm += io->counter;
        std::cout << "Feed Forward Send data " << total_comm << " Bytes. \n";
#endif
        return LongCiphertext();
    }
}
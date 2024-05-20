#include <model.h>

int main() {
    
}
/*
        size_t i, j;
        matrix W1(d_module * ffn_dim), b1(batch_size * ffn_dim), W2(ffn_dim * d_module), b2(batch_size * d_module);
        random_mat(W1);
        random_mat(b1);
        random_mat(W2);
        random_mat(b2);

        // send input to alice to encode
        LongPlaintext ln_plain = ln_secret_a.decrypt(alice);
        matrix ln = ln_plain.decode(encoder);
        LongCiphertext* ln_encode_secret_a = RFCP_encodeA(ln, alice, encoder, batch_size, d_module, ffn_dim);

        // send ln_encode_secret_a to bob
        LongCiphertext x1_secret_a = RFCP_matmul(ln_encode_secret_a, W1, batch_size, d_module, ffn_dim, encoder, evaluator);
        LongPlaintext parm0(5.075, encoder), parm1(sqrt(2), encoder), parm2(-sqrt(2), encoder), parm3(-5.075, encoder);
        LongCiphertext s0_secret_a = x1_secret_a.add_plain(parm0, evaluator);
        LongCiphertext s1_secret_a = x1_secret_a.add_plain(parm1, evaluator);
        LongCiphertext s2_secret_a = x1_secret_a.add_plain(parm2, evaluator);
        LongCiphertext s3_secret_a = x1_secret_a.add_plain(parm3, evaluator);
*/
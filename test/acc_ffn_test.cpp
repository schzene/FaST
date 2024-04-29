#include <module.h>
#define TEST

class SecureFFN {
    CKKSKey *alice;
    CKKSKey *bob;
    CKKSEncoder *encoder;
    Evaluator *evaluator;

    inline double gelu(double x) {
        return 0.5 * x * (1 + tanh(sqrt(2 / M_PI) * (x + 0.047715 * x * x * x)));
    }

    inline void activate(std::vector<double> &mat) {
        size_t size = mat.size();
        for (size_t i = 0; i < size; i++) {
            mat[i] = gelu(mat[i]);
        }
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
        std::vector<double> W1a(d_module * ffn_dim), W1b(d_module * ffn_dim),
            b1a(batch_size * ffn_dim), b1b(batch_size * ffn_dim),
            W2a(ffn_dim * d_module), W2b(ffn_dim * d_module),
            b2a(batch_size * d_module), b2b(batch_size * d_module),
            W1(d_module * ffn_dim), b1(batch_size * ffn_dim), W2(ffn_dim * d_module), b2(batch_size * d_module);
        random_mat(W1a);
        random_mat(W1b);
        random_mat(b1a);
        random_mat(b1b);
        random_mat(W2a);
        random_mat(W2b);
        random_mat(b2a);
        random_mat(b2b);
#ifdef TEST
        for (i = 0; i < ffn_dim * d_module; i++) {
            W1[i] = W1a[i] + W1b[i];
            W2[i] = W2a[i] + W2b[i];
        }
        for (i = 0; i < batch_size * ffn_dim; i++) {
            b1[i] = b1a[i] + b1b[i];
        }
        for (i = 0; i < batch_size * d_module; i++) {
            b2[i] = b2a[i] + b2b[i];
        }
#endif

        std::cout << "Secure Feed Forward done.\n";
#ifdef TEST
        auto ln_plain = ln_secret_a.decrypt(alice);
        auto ln = ln_plain.decode(encoder);
        auto x1 = matmul(ln, W1, batch_size, d_module, ffn_dim);
        for (i = 0; i < batch_size * ffn_dim; i++) {
            x1[i] += b1[i];
        }
        activate(x1);
        auto x2 = matmul(x1, W2, batch_size, ffn_dim, d_module);
        for (i = 0; i < batch_size * d_module; i++) {
            x2[i] += b2[i];
        }
        print_mat(x2, batch_size, d_module);
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
    std::vector<double> input(batch_size * d_module);
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
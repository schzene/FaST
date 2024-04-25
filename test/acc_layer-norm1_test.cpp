#include <module.h>

class SecureLayerNorm1 {
    CKKSKey *alice;
    CKKSKey *bob;
    CKKSEncoder *encoder;
    Evaluator *evaluator;
    std::vector<double> input;

    SecureLayerNorm1() {

    }

    ~SecureLayerNorm1() {

    }

    void forward(const LongCiphertext attn_s, const std::vector<double>& input) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(-1, 1);
        size_t i, j;
        std::vector<double> input_a(batch_size * d_module), input_b(batch_size * d_module);
        random_mat(input_b, 0, 0.01);
        for (i = 0; i < batch_size * d_module; i++) {
            input_a[i] = input[i] - input_b[i];
        }

        double hc1 = dist(gen), hc2 = dist(gen);
        std::vector<double> hc1_xc(input.size());
        for (i = 0; i < batch_size * d_module; i++) {
            hc1_xc[i] = hc1 * input_a[i];
        }
        LongPlaintext hc2_plain;
    }
};

int main() {

}
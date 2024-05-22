#include <model.h>

int main() {
    INIT_TIMER
    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    SEALContext *context = new SEALContext(parms);
    CKKSEncoder *encoder = new CKKSEncoder(*context);
    Evaluator *evaluator = new Evaluator(*context);

    CKKSKey *key = new CKKSKey(1, context);

    vector<double> list1(4096), list2(4096);
    random_mat(list1);
    random_mat(list2);

    START_TIMER
    Plaintext pt1, pt2;
    encoder->encode(list1, scale, pt1);
    encoder->encode(list2, scale, pt2);
    Ciphertext ct1, ct2;
    key->encryptor->encrypt(pt1, ct1);
    key->encryptor->encrypt(pt2, ct2);
    STOP_TIMER("encrypt")

    START_TIMER
    evaluator->add_inplace(ct1, ct2);
    STOP_TIMER("add")
}
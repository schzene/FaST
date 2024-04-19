#include <seal/seal.h>
using namespace seal;

class CKKSKey {
public:
    size_t slot_count;
    SEALContext* context;
    KeyGenerator* keygen;
    Encryptor* encryptor;
    Decryptor* decryptor;
    PublicKey public_key;
    
    CKKSKey(SEALContext* context_, size_t slot_count_);
    ~CKKSKey();
};

class LongPlaintext {
public:
    std::vector<Plaintext> plain_data;
    size_t len;
    size_t slot_count;
    CKKSEncoder* encoder;

    LongPlaintext(size_t slot_count_, CKKSEncoder* encoder_): slot_count(slot_count_), encoder(encoder_) {}
    LongPlaintext(Plaintext pt, size_t slot_count_, CKKSEncoder* encoder_);
    LongPlaintext(std::vector<double> data, double scale, size_t slot_count_, CKKSEncoder* encoder_);
    std::vector<double> decode();
    inline void mod_switch_to_inplace(parms_id_type parms_id, Evaluator* evaluator) {
        for (size_t i = 0; i < plain_data.size(); i++) 
            evaluator->mod_switch_to_inplace(plain_data[i], parms_id);
    }
};

class LongCiphertext {
public:
    std::vector<Ciphertext> cipher_data;
    size_t len;

    LongCiphertext() {} 
    LongCiphertext(Ciphertext ct);  
    LongCiphertext(LongPlaintext lpt, CKKSKey* party);
    LongPlaintext decrypt(CKKSEncoder* encoder, CKKSKey* party);
    void add_plain_inplace(LongPlaintext& lpt, Evaluator* evaluator);
    LongCiphertext multiply_plain(LongPlaintext& lpt, Evaluator* evaluator);
    void multiply_plain_inplace(LongPlaintext& lpt, Evaluator* evaluator);
    inline void rescale_to_next_inplace(Evaluator* evaluator) {
        for (size_t i = 0; i < cipher_data.size(); i++) {
            evaluator->rescale_to_next_inplace(cipher_data[i]);
        }
    }

    inline void scale(double scale_){
        for (size_t i = 0; i < cipher_data.size(); i++) {
            cipher_data[i].scale() = scale_;
        }
    }

    inline parms_id_type parms_id() {
        return cipher_data[0].parms_id();
    }
};
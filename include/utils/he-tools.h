#ifndef FAST_HE_TOOLS_H__
#define FAST_HE_TOOLS_H__
#include <cassert>
#include <sstream>
#include <string>

#include "SCI/utils.h" // prg.h & io & arg
#include <seal/seal.h>

typedef vector<double> matrix;

using std::string;
using std::vector;
using namespace seal;

const size_t poly_modulus_degree = 8192;
const size_t slot_count = poly_modulus_degree / 2;
const double scale = 1ul << 40;

class lenth_error : public std::exception {
    const char *message;

public:
    lenth_error(const char *msg) : message(msg) {}
    const char *what() const throw() override { return message; }
};

class CKKSKey {
public:
    int party;
    SEALContext *context;
    KeyGenerator *keygen;
    Encryptor *encryptor;
    Decryptor *decryptor;
    PublicKey public_key;
    RelinKeys relin_keys;
    CKKSKey(int party_, SEALContext *context_);
    ~CKKSKey();
};

class LongPlaintext {
public:
    vector<Plaintext> plain_data;
    size_t len;
    LongPlaintext() {}
    LongPlaintext(const Plaintext &pt);
    LongPlaintext(double data, CKKSEncoder *encoder);
    LongPlaintext(matrix data, CKKSEncoder *encoder);
    matrix decode(CKKSEncoder *encoder) const;

    inline void mod_switch_to_inplace(parms_id_type parms_id,
                                      Evaluator *evaluator) {
        for (size_t i = 0; i < plain_data.size(); i++) {
            evaluator->mod_switch_to_inplace(plain_data[i], parms_id);
        }
    }
};

class LongCiphertext {
public:
    vector<Ciphertext> cipher_data;
    size_t len;
    LongCiphertext() {}
    LongCiphertext(const Ciphertext &ct);
    LongCiphertext(double data, CKKSKey *party, CKKSEncoder *encoder);
    LongCiphertext(const LongPlaintext &lpt, CKKSKey *party);
    LongPlaintext decrypt(CKKSKey *party) const;
    void add_plain_inplace(LongPlaintext &lpt, Evaluator *evaluator);
    LongCiphertext add_plain(LongPlaintext &lpt, Evaluator *evaluator) const;
    void add_inplace(LongCiphertext &lct, Evaluator *evaluator);
    LongCiphertext add(LongCiphertext &lct, Evaluator *evaluator) const;
    void multiply_plain_inplace(LongPlaintext &lpt, Evaluator *evaluator,
                                RelinKeys *relin_keys = nullptr);
    LongCiphertext multiply_plain(LongPlaintext &lpt, Evaluator *evaluator,
                                  RelinKeys *relin_keys = nullptr) const;

    static void send(sci::NetIO *io, LongCiphertext *lct,
                     bool count_comm = true);
    static void recv(sci::NetIO *io, LongCiphertext *lct, SEALContext *context,
                     bool count_comm = true);

    inline void rescale_to_next_inplace(Evaluator *evaluator) {
        for (size_t i = 0; i < cipher_data.size(); i++) {
            evaluator->rescale_to_next_inplace(cipher_data[i]);
        }
    }

    inline void mod_switch_to_inplace(parms_id_type parms_id,
                                      Evaluator *evaluator) {
        for (size_t i = 0; i < cipher_data.size(); i++) {
            evaluator->mod_switch_to_inplace(cipher_data[i], parms_id);
        }
    }

    inline void rescale(double scale_) {
        for (size_t i = 0; i < cipher_data.size(); i++) {
            cipher_data[i].scale() = scale_;
        }
    }

    inline const parms_id_type parms_id() const noexcept {
        return cipher_data[0].parms_id();
    }

    inline void print_parameters(std::shared_ptr<seal::SEALContext> context) {
        // Verify parameters
        if (!context) {
            throw std::invalid_argument("context is not set");
        }
        auto &context_data = *context->key_context_data();

        /*
        Which scheme are we using?
        */
        std::string scheme_name;
        switch (context_data.parms().scheme()) {
        case seal::scheme_type::bfv:
            scheme_name = "BFV";
            break;
        case seal::scheme_type::ckks:
            scheme_name = "CKKS";
            break;
        default:
            throw std::invalid_argument("unsupported scheme");
        }
        std::cout << "/" << std::endl;
        std::cout << "| Encryption parameters :" << std::endl;
        std::cout << "|   scheme: " << scheme_name << std::endl;
        std::cout << "|   poly_modulus_degree: "
                  << context_data.parms().poly_modulus_degree() << std::endl;

        /*
        Print the size of the true (product) coefficient modulus.
        */
        std::cout << "|   coeff_modulus size: ";
        std::cout << context_data.total_coeff_modulus_bit_count() << " (";
        auto coeff_modulus = context_data.parms().coeff_modulus();
        std::size_t coeff_mod_count = coeff_modulus.size();
        for (std::size_t i = 0; i < coeff_mod_count - 1; i++) {
            std::cout << coeff_modulus[i].bit_count() << " + ";
        }
        std::cout << coeff_modulus.back().bit_count();
        std::cout << ") bits" << std::endl;

        /*
        For the BFV scheme print the plain_modulus parameter.
        */
        if (context_data.parms().scheme() == seal::scheme_type::bfv) {
            std::cout << "|   plain_modulus: "
                      << context_data.parms().plain_modulus().value()
                      << std::endl;
        }

        std::cout << "\\" << std::endl;
    }
};
#endif
#include "he-bfv.h"

void print_parameters(std::shared_ptr<seal::SEALContext> context) {
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
    std::cout << "|   poly_modulus_degree: " << context_data.parms().poly_modulus_degree() << std::endl;

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
        std::cout << "|   plain_modulus: " << context_data.parms().plain_modulus().value() << std::endl;
    }

    std::cout << "\\" << std::endl;
}

BFVParm::BFVParm(size_t poly_modulus_degree_,
                 vector<int> coeff_bit_sizes_,
                 uint64_t plain_mod_) : poly_modulus_degree(poly_modulus_degree_),
                                        slot_count(poly_modulus_degree_),
                                        coeff_bit_sizes(coeff_bit_sizes_),
                                        plain_mod(plain_mod_) {
    // Generate keys
    EncryptionParameters parms(scheme_type::bfv);

    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(
        CoeffModulus::Create(poly_modulus_degree, coeff_bit_sizes_));
    parms.set_plain_modulus(plain_mod);

    context = new SEALContext(parms, true, seal::sec_level_type::tc128);
    encoder = new BatchEncoder(*context);
    evaluator = new Evaluator(*context);
}

BFVParm::~BFVParm() {
    delete context;
    delete encoder;
    delete evaluator;
}

BFVKey::BFVKey(int party_, BFVParm *parm_) : party(party_), parm(parm_) {
    assert(party == sci::ALICE || party == sci::BOB);

    KeyGenerator *keygen = new KeyGenerator(*(parm->context));
    keygen->create_public_key(public_key);
    keygen->create_relin_keys(relin_keys);
    encryptor = new Encryptor(*(parm->context), public_key);
    decryptor = new Decryptor(*(parm->context), keygen->secret_key());
    delete keygen;
}

BFVKey::~BFVKey() {
    delete encryptor;
    delete decryptor;
}

BFVLongPlaintext::BFVLongPlaintext(const Plaintext &pt) {
    len = 1;
    plain_data.push_back(pt);
}

BFVLongPlaintext::BFVLongPlaintext(BFVParm *contex, uint64_t data) {
    // TODO value len =1, use the BFV batchencoder to encode the palaintext
    len = 1;
    Plaintext pt;
    vector<uint64_t> temp(contex->slot_count, data);
    contex->encoder->encode(temp, pt);
    plain_data.push_back(pt);
}

BFVLongPlaintext::BFVLongPlaintext(BFVParm *contex, bfv_matrix data) {
    len = data.size();
    size_t slot_count = contex->slot_count; // TODO:: this slot_count use SEALcontext? BFVLongPlaintext contain it.
    size_t count = len / slot_count;

    if (len % slot_count) {
        count++;
    }
    size_t i, j;
    if (slot_count >= len) {
        Plaintext pt;
        contex->encoder->encode(data, pt);
        plain_data.push_back(pt);
    } else {
        bfv_matrix::iterator curPtr = data.begin(), endPtr = data.end(), end;
        while (curPtr < endPtr) {
            end = endPtr - curPtr > slot_count ? slot_count + curPtr : endPtr;
            slot_count = endPtr - curPtr > slot_count ? slot_count : endPtr - curPtr;
            bfv_matrix temp(curPtr, end);
            Plaintext pt;
            contex->encoder->encode(temp, pt);
            plain_data.push_back(pt);
            curPtr += slot_count;
        }
    }
}

BFVLongPlaintext::BFVLongPlaintext(BFVParm *contex, uint64_t *data, size_t len) {
    this->len = len;
    size_t slot_count = contex->slot_count; // TODO:: this slot_count use SEALcontext? BFVLongPlaintext contain it.
    size_t count = len / slot_count;

    if (len % slot_count) {
        count++;
    }
    size_t i, j;
    if (slot_count >= len) {
        Plaintext pt;
        contex->encoder->encode(vector<uint64_t>(data, data + len), pt);
        plain_data.push_back(pt);
    } else {
        uint64_t *curPtr = data, *endPtr = data + len, *end;
        while (curPtr < endPtr) {
            end = endPtr - curPtr > slot_count ? slot_count + curPtr : endPtr;
            slot_count = endPtr - curPtr > slot_count ? slot_count : endPtr - curPtr;
            bfv_matrix temp(curPtr, end);
            Plaintext pt;
            contex->encoder->encode(temp, pt);
            plain_data.push_back(pt);
            curPtr += slot_count;
        }
    }
}

bfv_matrix BFVLongPlaintext::decode(BFVParm *contex) const {
    bfv_matrix data(len);
    size_t size = plain_data.size();
    size_t solut_cout = contex->slot_count;
    for (size_t i = 0; i < size; i++) {
        bfv_matrix temp;
        contex->encoder->decode(plain_data[i], temp);
        if (i < size - 1) {
            copy(temp.begin(), temp.end(), data.begin() + i * solut_cout);
        } else {
            size_t tail_len = len % solut_cout;
            tail_len = tail_len ? tail_len : solut_cout;
            copy(temp.begin(), temp.begin() + tail_len + 1, data.begin() + i * solut_cout);
        }
    }
    return data;
}

BFVLongCiphertext::BFVLongCiphertext(const Ciphertext &ct) {
    len = 1;
    cipher_data.push_back(ct);
}

BFVLongCiphertext::BFVLongCiphertext(BFVParm *contex, uint64_t data, BFVKey *party) {
    // TODO:
    len = 1;
    Plaintext pt;
    vector<uint64_t> temp(contex->slot_count, data);
    contex->encoder->encode(temp, pt);
    Ciphertext ct;
    party->encryptor->encrypt(pt, ct);
    cipher_data.push_back(ct);
}

BFVLongCiphertext::BFVLongCiphertext(const BFVLongPlaintext &lpt, BFVKey *party) {
    len = lpt.len;
    for (Plaintext pt : lpt.plain_data) {
        Ciphertext ct;
        party->encryptor->encrypt(pt, ct);
        cipher_data.push_back(ct);
    }
}

BFVLongPlaintext BFVLongCiphertext::decrypt(BFVKey *party) const {
    BFVLongPlaintext lpt;
    lpt.len = len;
    for (Ciphertext ct : cipher_data) {
        Plaintext pt;
        party->decryptor->decrypt(ct, pt);

        lpt.plain_data.push_back(pt);
    }
    return lpt;
}

void BFVLongCiphertext::add_plain_inplace(BFVLongPlaintext &lpt, Evaluator *evaluator) {
    if (len == 1) // cipher text len =1
    {
        len = lpt.len;
        Ciphertext ct(cipher_data[0]);
        cipher_data.pop_back();
        for (Plaintext pt : lpt.plain_data) {
            Ciphertext ctemp;
            evaluator->add_plain(ct, pt, ctemp);
            cipher_data.push_back(ctemp);
        }
    } else if (lpt.len == 1) {
        for (size_t i = 0; i < cipher_data.size(); i++)
            evaluator->add_plain_inplace(cipher_data[i], lpt.plain_data[0]);
    } else if (len == lpt.len) {
        for (size_t i = 0; i < cipher_data.size(); i++)
            evaluator->add_plain_inplace(cipher_data[i], lpt.plain_data[i]);
    } else {
        char buf[100];
        sprintf(buf, "Length of BFVLongCiphertext(%ld) and BFVLongPlaintext(%ld) mismatch", len, lpt.len);
        throw bfv_lenth_error(buf);
    }
}

BFVLongCiphertext BFVLongCiphertext::add_plain(BFVLongPlaintext &lpt, Evaluator *evaluator) const {
    {
        BFVLongCiphertext lct;
        lct.len = 0;
        if (len == 1) {
            lct.len = lpt.len;
            for (size_t i = 0; i < lpt.plain_data.size(); i++) {
                Ciphertext ct;
                evaluator->add_plain(cipher_data[0], lpt.plain_data[i], ct);
                lct.cipher_data.push_back(ct);
            }
        } else if (lpt.len == 1) {
            lct.len = len;
            for (size_t i = 0; i < cipher_data.size(); i++) {
                Ciphertext ct;
                evaluator->add_plain(cipher_data[i], lpt.plain_data[0], ct);
                lct.cipher_data.push_back(ct);
            }
        } else if (len == lpt.len) {
            lct.len = len;
            for (size_t i = 0; i < cipher_data.size(); i++) {
                Ciphertext ct;
                evaluator->add_plain(cipher_data[i], lpt.plain_data[i], ct);
                lct.cipher_data.push_back(ct);
            }
        } else {
            char buf[100];
            sprintf(buf, "Length of BFVLongCiphertext(%ld) and LongPlaintext(%ld) mismatch", len, lpt.len);
            throw bfv_lenth_error(buf);
        }
        return lct;
    }
}

void BFVLongCiphertext::add_inplace(BFVLongCiphertext &lct, Evaluator *evaluator) {
    if (len == 1) {
        len = lct.len;
        Ciphertext ct(cipher_data[0]);
        cipher_data.pop_back();
        for (Ciphertext cct : lct.cipher_data) {
            Ciphertext ctemp;
            evaluator->add(ct, cct, ctemp);
            cipher_data.push_back(ctemp);
        }
    } else if (lct.len == 1) {
        for (size_t i = 0; i < cipher_data.size(); i++)
            evaluator->add_inplace(cipher_data[i], lct.cipher_data[0]);
    } else if (len == lct.len) {
        for (size_t i = 0; i < cipher_data.size(); i++)
            evaluator->add_inplace(cipher_data[i], lct.cipher_data[i]);
    } else {
        char buf[100];
        sprintf(buf, "Length of BFVLongCiphertext(%ld) and BFVLongCiphertext(%ld) mismatch", len, lct.len);
        throw bfv_lenth_error(buf);
    }
}

BFVLongCiphertext BFVLongCiphertext::add(BFVLongCiphertext &lct, Evaluator *evaluator) const {
    BFVLongCiphertext lcct;
    lcct.len = 0;
    if (len == 1) {
        lcct.len = lct.len;
        for (size_t i = 0; i < lct.cipher_data.size(); i++) {
            Ciphertext ct;
            evaluator->add(cipher_data[0], lct.cipher_data[i], ct);
            lcct.cipher_data.push_back(ct);
        }
    } else if (lct.len == 1) {
        lcct.len = len;
        for (size_t i = 0; i < cipher_data.size(); i++) {
            Ciphertext ct;
            evaluator->add(cipher_data[i], lct.cipher_data[0], ct);
            lcct.cipher_data.push_back(ct);
        }
    } else if (len == lct.len) {
        lcct.len = len;
        for (size_t i = 0; i < cipher_data.size(); i++) {
            Ciphertext ct;
            evaluator->add(cipher_data[i], lct.cipher_data[i], ct);
            lcct.cipher_data.push_back(ct);
        }
    } else {
        char buf[100];
        sprintf(buf, "Length of BFVLongCiphertext(%ld) and BFVLongCiphertext(%ld) mismatch", len, lct.len);
        throw bfv_lenth_error(buf);
    }
    return lcct;
}

void BFVLongCiphertext::multiply_plain_inplace(BFVLongPlaintext &lpt, Evaluator *evaluator, RelinKeys *relin_keys) {

    if (len == 1) {
        len = lpt.len;

        Ciphertext ct(cipher_data[0]);
        cipher_data.pop_back();
        for (Plaintext pt : lpt.plain_data) {
            Ciphertext ctemp;

            evaluator->multiply_plain(ct, pt, ctemp);

            if (relin_keys != nullptr) {
                evaluator->relinearize_inplace(ctemp, *relin_keys);
            }

            cipher_data.push_back(ctemp);
        }
    } else if (lpt.len == 1) {
        for (size_t i = 0; i < cipher_data.size(); i++) {
            evaluator->multiply_plain_inplace(cipher_data[i], lpt.plain_data[0]);
            if (relin_keys != nullptr) {
                evaluator->relinearize_inplace(cipher_data[i], *relin_keys);
            }
        }
    } else if (len == lpt.len) {
        for (size_t i = 0; i < cipher_data.size(); i++) {
            evaluator->multiply_plain_inplace(cipher_data[i], lpt.plain_data[i]);
            if (relin_keys != nullptr) {
                evaluator->relinearize_inplace(cipher_data[i], *relin_keys);
            }
        }
    } else {
        char buf[100];
        sprintf(buf, "Length of LongCiphertext(%ld) and LongPlaintext(%ld) mismatch", len, lpt.len);
        throw bfv_lenth_error(buf);
    }
}

BFVLongCiphertext BFVLongCiphertext::multiply_plain(BFVLongPlaintext &lpt, Evaluator *evaluator, RelinKeys *relin_keys) const {
    BFVLongCiphertext lct;
    lct.len = 0;
    if (len == 1) {
        lct.len = lpt.len;
        for (size_t i = 0; i < lpt.plain_data.size(); i++) {
            Ciphertext ct;
            evaluator->multiply_plain(cipher_data[0], lpt.plain_data[i], ct);
            if (relin_keys != nullptr) {
                evaluator->relinearize_inplace(ct, *relin_keys);
            }
            lct.cipher_data.push_back(ct);
        }
    } else if (lpt.len == 1) {
        lct.len = len;
        for (size_t i = 0; i < cipher_data.size(); i++) {
            Ciphertext ct;
            evaluator->multiply_plain(cipher_data[i], lpt.plain_data[0], ct);
            if (relin_keys != nullptr) {
                evaluator->relinearize_inplace(ct, *relin_keys);
            }

            lct.cipher_data.push_back(ct);
        }
    } else if (len == lpt.len) {
        lct.len = len;
        for (size_t i = 0; i < cipher_data.size(); i++) {
            Ciphertext ct;
            evaluator->multiply_plain(cipher_data[i], lpt.plain_data[i], ct);
            if (relin_keys != nullptr) {
                evaluator->relinearize_inplace(ct, *relin_keys);
            }

            lct.cipher_data.push_back(ct);
        }
    } else {
        char buf[100];
        sprintf(buf, "Length of BFVLongCiphertext(%ld) and BFVLongPlaintext(%ld) mismatch", len, lpt.len);
        throw bfv_lenth_error(buf);
    }
    return lct;
}

void BFVLongCiphertext::send(sci::NetIO *io, BFVLongCiphertext *lct) {
    assert(lct->len > 0);
    io->send_data(&(lct->len), sizeof(size_t));
    size_t size = lct->cipher_data.size();
    io->send_data(&size, sizeof(size_t));
    for (size_t ct = 0; ct < size; ct++) {
        std::stringstream os;
        uint64_t ct_size;
        lct->cipher_data[ct].save(os);
        ct_size = os.tellp();
        string ct_ser = os.str();
        io->send_data(&ct_size, sizeof(uint64_t));
        io->send_data(ct_ser.c_str(), ct_ser.size());
    }
}

void BFVLongCiphertext::recv(sci::NetIO *io, BFVLongCiphertext *lct, SEALContext *context) {
    io->recv_data(&(lct->len), sizeof(size_t));
    size_t size;
    io->recv_data(&size, sizeof(size_t));
    for (size_t ct = 0; ct < size; ct++) {
        Ciphertext cct;
        std::stringstream is;
        uint64_t ct_size;
        io->recv_data(&ct_size, sizeof(uint64_t));
        char *c_enc_result = new char[ct_size];
        io->recv_data(c_enc_result, ct_size);
        is.write(c_enc_result, ct_size);
        cct.unsafe_load(*context, is);
        lct->cipher_data.push_back(cct);
        delete[] c_enc_result;
    }
}
#include "he-tools.h"

CKKSKey::CKKSKey(int party_, SEALContext *context_) : party(party_), context(context_) {
    assert(party == sci::ALICE || party == sci::BOB);
    keygen = new KeyGenerator(*context);
    keygen->create_public_key(public_key);
    keygen->create_relin_keys(relin_keys);
    encryptor = new Encryptor(*context, public_key);
    decryptor = new Decryptor(*context, keygen->secret_key());
}

CKKSKey::~CKKSKey() {
    delete keygen;
    delete encryptor;
    delete decryptor;
}

LongPlaintext::LongPlaintext(const Plaintext &pt) {
    len = 1;
    plain_data.push_back(pt);
}

LongPlaintext::LongPlaintext(double data, CKKSEncoder *encoder) {
    len = 1;
    Plaintext pt;
    encoder->encode(data, scale, pt);
    plain_data.push_back(pt);
}

LongPlaintext::LongPlaintext(matrix data, CKKSEncoder *encoder) {
    len = data.size();
    size_t slot = slot_count;
    size_t count = len / slot;
    if (len % slot) {
        count++;
    }
    size_t i, j;
    if (slot >= len) {
        Plaintext pt;
        encoder->encode(data, scale, pt);
        plain_data.push_back(pt);
    } else {
        matrix::iterator curPtr = data.begin(), endPtr = data.end(), end;
        while (curPtr < endPtr) {
            end = endPtr - curPtr > slot ? slot + curPtr : endPtr;
            slot = endPtr - curPtr > slot ? slot : endPtr - curPtr;
            matrix temp(curPtr, end);
            Plaintext pt;
            encoder->encode(temp, scale, pt);
            plain_data.push_back(pt);
            curPtr += slot;
        }
    }
}

matrix LongPlaintext::decode(CKKSEncoder *encoder) const {
    matrix data(len);
    size_t size = plain_data.size();
    for (size_t i = 0; i < size; i++) {
        matrix temp;
        encoder->decode(plain_data[i], temp);
        if (i < size - 1) {
            copy(temp.begin(), temp.end(), data.begin() + i * slot_count);
        } else {
            size_t tail_len = len % slot_count;
            tail_len = tail_len ? tail_len : slot_count;
            copy(temp.begin(), temp.begin() + tail_len, data.begin() + i * slot_count);
        }
    }
    return data;
}

LongCiphertext::LongCiphertext(const Ciphertext &ct) {
    len = 1;
    cipher_data.push_back(ct);
}

LongCiphertext::LongCiphertext(double data, CKKSKey *party, CKKSEncoder *encoder) {
    len = 1;
    Plaintext pt;
    encoder->encode(data, scale, pt);
    Ciphertext ct;
    party->encryptor->encrypt(pt, ct);
    cipher_data.push_back(ct);
}

LongCiphertext::LongCiphertext(const LongPlaintext &lpt, CKKSKey *party) {
    len = lpt.len;
    for (Plaintext pt : lpt.plain_data) {
        Ciphertext ct;
        party->encryptor->encrypt(pt, ct);
        cipher_data.push_back(ct);
    }
}

LongPlaintext LongCiphertext::decrypt(CKKSKey *party) const {
    LongPlaintext lpt;
    lpt.len = len;
    for (Ciphertext ct : cipher_data) {
        Plaintext pt;
        party->decryptor->decrypt(ct, pt);
        lpt.plain_data.push_back(pt);
    }
    return lpt;
}

void LongCiphertext::add_plain_inplace(LongPlaintext &lpt, Evaluator *evaluator) {
    if (len == 1) {
        len = lpt.len;
        Ciphertext ct(cipher_data[0]);
        cipher_data.pop_back();
        for (Plaintext pt : lpt.plain_data) {
            Ciphertext ctemp;
            evaluator->add_plain(ct, pt, ctemp);
            cipher_data.push_back(ctemp);
        }
    } else if (lpt.len == 1) {
        for (size_t i = 0; i < cipher_data.size(); i++) {
            evaluator->add_plain_inplace(cipher_data[i], lpt.plain_data[0]);
        }
    } else if (len == lpt.len) {
        for (size_t i = 0; i < cipher_data.size(); i++) {
            evaluator->add_plain_inplace(cipher_data[i], lpt.plain_data[i]);
        }
    } else {
        char buf[100];
        sprintf(buf, "Length of LongCiphertext(%ld) and LongPlaintext(%ld) mismatch", len, lpt.len);
        throw lenth_error(buf);
    }
}

LongCiphertext LongCiphertext::add_plain(LongPlaintext &lpt, Evaluator *evaluator) const {
    LongCiphertext lct;
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
        sprintf(buf, "Length of LongCiphertext(%ld) and LongPlaintext(%ld) mismatch", len, lpt.len);
        throw lenth_error(buf);
    }
    return lct;
}

void LongCiphertext::add_inplace(LongCiphertext &lct, Evaluator *evaluator) {
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
        for (size_t i = 0; i < cipher_data.size(); i++) {
            evaluator->add_inplace(cipher_data[i], lct.cipher_data[0]);
        }
    } else if (len == lct.len) {
        for (size_t i = 0; i < cipher_data.size(); i++) {
            evaluator->add_inplace(cipher_data[i], lct.cipher_data[i]);
        }
    } else {
        char buf[100];
        sprintf(buf, "Length of LongCiphertext(%ld) and LongCiphertext(%ld) mismatch", len, lct.len);
        throw lenth_error(buf);
    }
}

LongCiphertext LongCiphertext::add(LongCiphertext &lct, Evaluator *evaluator) const {
    LongCiphertext lcct;
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
        sprintf(buf, "Length of LongCiphertext(%ld) and LongCiphertext(%ld) mismatch", len, lct.len);
        throw lenth_error(buf);
    }
    return lcct;
}

void LongCiphertext::multiply_plain_inplace(LongPlaintext &lpt, Evaluator *evaluator, RelinKeys *relin_keys) {
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
            evaluator->rescale_to_next_inplace(ctemp);
            ctemp.scale() = scale;
            cipher_data.push_back(ctemp);
        }
    } else if (lpt.len == 1) {
        for (size_t i = 0; i < cipher_data.size(); i++) {
            evaluator->multiply_plain_inplace(cipher_data[i], lpt.plain_data[0]);
            if (relin_keys != nullptr) {
                evaluator->relinearize_inplace(cipher_data[i], *relin_keys);
            }
            evaluator->rescale_to_next_inplace(cipher_data[i]);
            cipher_data[i].scale() = scale;
        }
    } else if (len == lpt.len) {
        for (size_t i = 0; i < cipher_data.size(); i++) {
            evaluator->multiply_plain_inplace(cipher_data[i], lpt.plain_data[i]);
            if (relin_keys != nullptr) {
                evaluator->relinearize_inplace(cipher_data[i], *relin_keys);
            }
            evaluator->rescale_to_next_inplace(cipher_data[i]);
            cipher_data[i].scale() = scale;
        }
    } else {
        char buf[100];
        sprintf(buf, "Length of LongCiphertext(%ld) and LongPlaintext(%ld) mismatch", len, lpt.len);
        throw lenth_error(buf);
    }
}

LongCiphertext LongCiphertext::multiply_plain(LongPlaintext &lpt, Evaluator *evaluator, RelinKeys *relin_keys) const {
    LongCiphertext lct;
    lct.len = 0;
    if (len == 1) {
        lct.len = lpt.len;
        for (size_t i = 0; i < lpt.plain_data.size(); i++) {
            Ciphertext ct;
            evaluator->multiply_plain(cipher_data[0], lpt.plain_data[i], ct);
            if (relin_keys != nullptr) {
                evaluator->relinearize_inplace(ct, *relin_keys);
            }
            evaluator->rescale_to_next_inplace(ct);
            ct.scale() = scale;
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
            evaluator->rescale_to_next_inplace(ct);
            ct.scale() = scale;
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
            evaluator->rescale_to_next_inplace(ct);
            ct.scale() = scale;
            lct.cipher_data.push_back(ct);
        }
    } else {
        char buf[100];
        sprintf(buf, "Length of LongCiphertext(%ld) and LongPlaintext(%ld) mismatch", len, lpt.len);
        throw lenth_error(buf);
    }
    return lct;
}

void LongCiphertext::send(sci::NetIO *io, LongCiphertext *lct, bool count_comm) {
    assert(lct->len > 0);
    io->send_data(&(lct->len), sizeof(size_t), count_comm);
    size_t size = lct->cipher_data.size();
    io->send_data(&size, sizeof(size_t), count_comm);
    for (size_t ct = 0; ct < size; ct++) {
        std::stringstream os;
        uint64_t ct_size;
        lct->cipher_data[ct].save(os);
        ct_size = os.tellp();
        string ct_ser = os.str();
        io->send_data(&ct_size, sizeof(uint64_t), count_comm);
        io->send_data(ct_ser.c_str(), ct_ser.size(), count_comm);
    }
}

void LongCiphertext::recv(sci::NetIO *io, LongCiphertext *lct, SEALContext *context, bool count_comm) {
    io->recv_data(&(lct->len), sizeof(size_t), count_comm);
    size_t size;
    io->recv_data(&size, sizeof(size_t), count_comm);
    for (size_t ct = 0; ct < size; ct++) {
        Ciphertext cct;
        std::stringstream is;
        uint64_t ct_size;
        io->recv_data(&ct_size, sizeof(uint64_t), count_comm);
        char *c_enc_result = new char[ct_size];
        io->recv_data(c_enc_result, ct_size, count_comm);
        is.write(c_enc_result, ct_size);
        cct.unsafe_load(*context, is);
        lct->cipher_data.push_back(cct);
        delete[] c_enc_result;
    }
}
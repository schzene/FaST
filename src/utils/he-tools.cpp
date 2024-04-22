#include "he-tools.h"

CKKSKey::CKKSKey(SEALContext *context_, size_t slot_count_) : context(context_), slot_count(slot_count_) {
    keygen = new KeyGenerator(*context);
    keygen->create_public_key(public_key);
    encryptor = new Encryptor(*context, public_key);
    decryptor = new Decryptor(*context, keygen->secret_key());
}

CKKSKey::~CKKSKey() {
    delete keygen;
    delete encryptor;
    delete decryptor;
}

LongPlaintext::LongPlaintext(Plaintext pt, size_t slot_count_, CKKSEncoder *encoder_) : slot_count(slot_count_), encoder(encoder_) {
    len = 1;
    plain_data.push_back(pt);
}

LongPlaintext::LongPlaintext(std::vector<double> data, double scale, size_t slot_count_, CKKSEncoder *encoder_) : slot_count(slot_count_), encoder(encoder_) {
    len = data.size();
    size_t count = len / slot_count;
    if (len % slot_count) {
        count++;
    }
    size_t i, j;
    if (slot_count >= len) {
        Plaintext pt;
        encoder->encode(data, scale, pt);
        plain_data.push_back(pt);
    } else {
        std::vector<double>::iterator curPtr = data.begin(), endPtr = data.end(), end;
        while (curPtr < endPtr) {
            end = endPtr - curPtr > slot_count ? slot_count + curPtr : endPtr;
            slot_count = endPtr - curPtr > slot_count ? slot_count : endPtr - curPtr;
            std::vector<double> temp(curPtr, end);
            Plaintext pt;
            encoder->encode(temp, scale, pt);
            plain_data.push_back(pt);
            curPtr += slot_count;
        }
    }
}

std::vector<double> LongPlaintext::decode() {
    std::vector<double> data(len);
    size_t size = plain_data.size();
    for (size_t i = 0; i < size; i++) {
        std::vector<double> temp;
        encoder->decode(plain_data[i], temp);
        if (i < size - 1) {
            std::copy(temp.begin(), temp.end(), data.begin() + i * slot_count);
        } else {
            size_t tail_len = len % slot_count;
            tail_len = tail_len ? tail_len : slot_count;
            std::copy(temp.begin(), temp.begin() + tail_len + 1, data.begin() + i * slot_count);
        }
    }
    return data;
}

LongCiphertext::LongCiphertext(Ciphertext ct) {
    len = 1;
    cipher_data.push_back(ct);
}

LongCiphertext::LongCiphertext(LongPlaintext lpt, CKKSKey *party) {
    len = lpt.len;
    for (Plaintext pt : lpt.plain_data) {
        Ciphertext ct;
        party->encryptor->encrypt(pt, ct);
        cipher_data.push_back(ct);
    }
}

LongPlaintext LongCiphertext::decrypt(CKKSEncoder *encoder, CKKSKey *party) {
    LongPlaintext lpt(party->slot_count, encoder);
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
        for (size_t i = 0; i < cipher_data.size(); i++)
            evaluator->add_plain_inplace(cipher_data[i], lpt.plain_data[0]);
    } else if (len == lpt.len) {
        for (size_t i = 0; i < cipher_data.size(); i++)
            evaluator->add_plain_inplace(cipher_data[i], lpt.plain_data[i]);
    }
}

LongCiphertext LongCiphertext::multiply_plain(LongPlaintext &lpt, Evaluator *evaluator) {
    LongCiphertext lct;
    lct.len = 0;
    if (len == 1) {
        lct.len = lpt.len;
        for (size_t i = 0; i < lpt.plain_data.size(); i++) {
            Ciphertext ct;
            evaluator->multiply_plain(cipher_data[0], lpt.plain_data[i], ct);
            lct.cipher_data.push_back(ct);
        }
    } else if (lpt.len == 1) {
        lct.len = len;
        for (size_t i = 0; i < cipher_data.size(); i++) {
            Ciphertext ct;
            evaluator->multiply_plain(cipher_data[i], lpt.plain_data[0], ct);
            lct.cipher_data.push_back(ct);
        }
    } else if (len == lpt.len) {
        lct.len = len;
        for (size_t i = 0; i < cipher_data.size(); i++) {
            Ciphertext ct;
            evaluator->multiply_plain(cipher_data[i], lpt.plain_data[i], ct);
            lct.cipher_data.push_back(ct);
        }
    }
    return lct;
}

void LongCiphertext::multiply_plain_inplace(LongPlaintext &lpt, Evaluator *evaluator) {
    if (len == 1) {
        len = lpt.len;
        Ciphertext ct(cipher_data[0]);
        cipher_data.pop_back();
        for (Plaintext pt : lpt.plain_data) {
            Ciphertext ctemp;
            evaluator->multiply_plain(ct, pt, ctemp);
            cipher_data.push_back(ctemp);
        }
    } else if (lpt.len == 1) {
        for (size_t i = 0; i < cipher_data.size(); i++)
            evaluator->multiply_plain_inplace(cipher_data[i], lpt.plain_data[0]);
    } else if (len == lpt.len) {
        for (size_t i = 0; i < cipher_data.size(); i++)
            evaluator->multiply_plain_inplace(cipher_data[i], lpt.plain_data[i]);
    }
}
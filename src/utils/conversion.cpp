#include "conversion.h"

void Conversion::he_to_ss(BFVLongCiphertext &ciphertext, uint64_t *share, Evaluator *evaluator) {
    size_t len = ciphertext.len;
    ;
    BFVLongPlaintext p_2_plain(bfvparm, bfvparm->plain_mod / 2, encoder);
    BFVLongCiphertext lct = ciphertext.add_plain(p_2_plain, evaluator);
    // he->encoder->encode(p_2, pt_p_2);
    BFVLongPlaintext lpt = ciphertext.decrypt(party);
    bfv_matrix mat = lpt.decode(bfvparm, encoder);
    std::copy(mat.begin(), mat.end(), share);
}

void Conversion::ss_to_he(uint64_t *share, BFVLongCiphertext &ciphertext, int length, int bw) {
#ifdef LOG
    auto t_conversion = high_resolution_clock::now();
#endif
    int slot_count = bfvparm->slot_count;
    uint64_t plain_mod = bfvparm->plain_mod;
    int dim = length / slot_count;
    vector<uint64_t> tmp_plain(length);
    // bfv_matrix tmp_plain(length);
    for (size_t i = 0; i < dim; i++) {
        vector<uint64_t> tmp(slot_count);
        for (int j = 0; j < slot_count; ++j) {
            tmp_plain[i * slot_count + j] = sci::neg_mod(sci::signed_val(share[i * slot_count + j], bw), (int64_t)plain_mod);
        }
    }

    BFVLongPlaintext bfv_temp_plain(bfvparm, tmp_plain, encoder);
    ciphertext = BFVLongCiphertext(bfv_temp_plain, party);

#ifdef LOG
    t_total_conversion += interval(t_conversion);
#endif
}
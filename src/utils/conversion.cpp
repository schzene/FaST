#include "conversion.h"

Conversion::Conversion(BFVparm *bfvparm) {
    this->party = bfvparm->party;
}

Conversion::~Conversion() {
    delete *bfvparm;
}

void he_to_ss(BFVparm *bfvparm, BFVLongCiphertext *ciphertext, uint64_t *share, bool ring) {
}

void Conversion::ss_to_he(BFVparm *bfvparm, BFVKey *bfvkey, uint64_t *share, BFVLongCiphertext *ciphertext, int length, int bw) {
#ifdef LOG
    auto t_conversion = high_resolution_clock::now();
#endif
    int slot_count = bfvparm->bfv_slot_count;
    uint64_t plain_mod = bfvparm->bfv_plain_mod;
    int dim = length / slot_count;
    vector<uint64_t> tmp_plain(length);
    // bfv_matrix tmp_plain(length);
    for (size_t i = 0; i < dim; i++) {
        vector<uint64_t> tmp(slot_count);
        for (int j = 0; j < slot_count; ++j) {
            tmp[j] = neg_mod(signed_val(share[i * slot_count + j], bw), (int64_t)plain_mod);
        }
        tmp_plain.push_back(tmp);
    }

    BFVLongPlaintext bfv_temp_plain(bfvparm, tmp_plain, encoder);
    ciphertext = BFVLongCiphertext ct(bfv_temp_plain, bfvkey);

#ifdef LOG
    t_total_conversion += interval(t_conversion);
#endif
}
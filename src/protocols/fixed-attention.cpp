#include "fixed-attention.h"

bfv_matrix FixedAttention::forward(const bfv_matrix &input) const {
    size_t total_comm = 0;
    size_t i, j;
    size_t d_k = d_module / n_heads;
    bfv_matrix WQ(d_module * d_k), WK(d_module * d_k), WV(d_module * d_k);
    random_bfv_mat(WQ);
    random_bfv_mat(WK);
    random_bfv_mat(WV); // need be changed later
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-1, 1);

    if (party->party == sci::ALICE) {
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif
        double ra = dist(gen);
        bfv_matrix ra_xa(batch_size * d_module);
    } else {
    }
}
#include "ffn.h"

LongCiphertext FFN::forward(const LongCiphertext &ln1) {
    if (party->party == ALICE) {
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif

#ifdef LOG
        STOP_TIMER("Feed Forward")
#endif
    } else {
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif

#ifdef LOG
        STOP_TIMER("Feed Forward")
#endif
    }
}
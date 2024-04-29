#include "layer-norm2.h"

void LayerNorm2::forward() {
    if (party->party == ALICE) {
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif

#ifdef LOG
        STOP_TIMER("Layer Norm2 ")
#endif
    } else {
#ifdef LOG
        INIT_TIMER
        START_TIMER
#endif

#ifdef LOG
        STOP_TIMER("Layer Norm2 ")
#endif
    }
}
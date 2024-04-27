#include "layer-norm2.h"

void LayerNorm2::forward() {
    if (party->party == ALICE) {
#ifdef LOG
        std::cout << "Secure LayerNorm2 done.\n";
#endif
    } else {
#ifdef LOG
        std::cout << "Secure LayerNorm2 done.\n";
#endif
    }
}
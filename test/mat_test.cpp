#include <model.h>
INIT_TIMER
int main() {
    matrix A(batch_size, 10);
    int step = 1000000000;
    START_TIMER
    double *A_data = A.data();
    for (int i = 0; i < step; i++) {
        A_data[i % i]; // _M_data_ptr(this->_M_impl._M_start)
    }
    STOP_TIMER("access by data()")

    START_TIMER
    for (int i = 0; i < step; i++) {
        A[i % i];
    }
    STOP_TIMER("access directory")
}
#include <model.h>
#include <ctime>

int main() {
    INIT_TIMER
    START_TIMER
    sleep(1);
    PAUSE_TIMER("test", false)
    sleep(1);
    // PAUSE_TIMER("test");
    START_TIMER
    sleep(1);
    STOP_TIMER("test")
}
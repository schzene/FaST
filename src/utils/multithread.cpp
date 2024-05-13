#include "multithread.h"

mutex mtx;

void for_acc(const size_t start, const size_t end,
             const std::function<void(size_t, size_t, mutex *)> &for_range,
             const int max_threads) {
    const size_t step = (end - start) / max_threads + 1;
    std::vector<thread> threads;

    for (int i = 0; i < max_threads; i++) {
        size_t thread_start = start + i * step;
        if (thread_start > end) {
            break;
        }
        size_t thread_end = std::min(thread_start + step, end);
        threads.push_back(thread(for_range, thread_start, thread_end, &mtx));
    }
    
    for (thread &t: threads) {
        t.join();
    }
}

void multi_thread(const std::function<void(mutex *)> &task, const int thread_count) {

}
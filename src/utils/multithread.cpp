#include "multithread.h"

mutex mtx;

void for_acc(const size_t start, const size_t end,
             const std::function<void(size_t, size_t, mutex *)> &for_range,
             const int threads_count) {
    const size_t step = (end - start) / threads_count + 1;
    thread *threads = new thread[threads_count];

    for (int i = 0; i < threads_count; i++) {
        size_t thread_start = start + i * step;
        size_t thread_end = std::min(thread_start + step, end);
        threads[i] = thread(for_range, thread_start, thread_end, &mtx);
    }
    for (int i = 0; i < threads_count; i++) {
        threads[i].join();
    }

    delete[] threads;
}
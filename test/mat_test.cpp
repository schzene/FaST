#include <iostream>
#include <functional>
#include <thread>

using std::thread;

void for_acc(const size_t start, const size_t end,
             const std::function<void(size_t, size_t)> &for_range, const int threads_count = 4) {
    const size_t step = (end - start) / threads_count + 1;
    thread *threads = new thread[threads_count];

    for (int i = 0; i < threads_count; ++i) {
        size_t thread_start = start + i * step;
        size_t thread_end = std::min(thread_start + step, end);
        threads[i] = thread(for_range, thread_start, thread_end);
    }
    for (int i = 0; i < threads_count; ++i) {
        threads[i].join();
    }

    delete[] threads;
}

int count = 0;
void process_range(int start, int end) {
    for (int i = start; i < end; i++) {
        count += i;
    }
}

int main() {
    const int start = 1, end = 1001;
    for_acc(start, end, process_range, 4);
    std::cout << count << "\n";
    return 0;
}
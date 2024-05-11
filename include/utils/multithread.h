#ifndef FAST_MULTITHREAD_H__
#define FAST_MULTITHREAD_H__
#include <functional>
#include <mutex>
#include <thread>

using std::mutex;
using std::thread;

void for_acc(const size_t start, const size_t end,
             const std::function<void(size_t, size_t, mutex *)> &for_range,
             const int threads_count = 16);
#endif
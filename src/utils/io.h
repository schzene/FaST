#ifndef FAST_IO_H__
#define FAST_IO_H__
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
using std::string;

#include <arpa/inet.h>
#include <immintrin.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
using block = __m128i;

#include "group.h"

const static int HASH_BUFFER_SIZE = 1024 * 8;
const static int NETWORK_BUFFER_SIZE2 = 1024 * 32;
const static int NETWORK_BUFFER_SIZE = 1024 * 1024;
const static int FILE_BUFFER_SIZE = 1024 * 16;
const static int CHECK_BUFFER_SIZE = 1024 * 8;

inline void error(const char *s, int line = 0, const char *file = (const char *)nullptr) {
    fprintf(stderr, s, "\n");
    if (file != nullptr) {
        fprintf(stderr, "at %d, %s\n", line, file);
    }
    exit(1);
}

template <typename T>
class IOChannel {
public:
    uint64_t counter = 0;
    void send_data(const void *data, size_t nbyte) {
        counter += nbyte;
        derived().send_data_internal(data, nbyte);
    }

    void recv_data(void *data, size_t nbyte) {
        derived().recv_data_internal(data, nbyte);
    }

    void send_block(const block *data, size_t nblock) {
        send_data(data, nblock * sizeof(block));
    }

    void recv_block(block *data, size_t nblock) {
        recv_data(data, nblock * sizeof(block));
    }

    void send_pt(Point *A, size_t num_pts = 1) {
        for (size_t i = 0; i < num_pts; ++i) {
            size_t len = A[i].size();
            A[i].group->resize_scratch(len);
            unsigned char *tmp = A[i].group->scratch;
            send_data(&len, 4);
            A[i].to_bin(tmp, len);
            send_data(tmp, len);
        }
    }

    void recv_pt(Group *g, Point *A, size_t num_pts = 1) {
        size_t len = 0;
        for (size_t i = 0; i < num_pts; ++i) {
            recv_data(&len, 4);
            assert(len <= 2048);
            g->resize_scratch(len);
            unsigned char *tmp = g->scratch;
            recv_data(tmp, len);
            A[i].from_bin(g, tmp, len);
        }
    }

    void send_bool(bool *data, size_t length) {
        void *ptr = (void *)data;
        size_t space = length;
        const void *aligned = std::align(alignof(uint64_t), sizeof(uint64_t), ptr, space);
        if (aligned == nullptr)
            send_data(data, length);
        else {
            size_t diff = length - space;
            send_data(data, diff);
            send_bool_aligned((const bool *)aligned, length - diff);
        }
    }

    void recv_bool(bool *data, size_t length) {
        void *ptr = (void *)data;
        size_t space = length;
        void *aligned = std::align(alignof(uint64_t), sizeof(uint64_t), ptr, space);
        if (aligned == nullptr)
            recv_data(data, length);
        else {
            size_t diff = length - space;
            recv_data(data, diff);
            recv_bool_aligned((bool *)aligned, length - diff);
        }
    }

    void send_bool_aligned(const bool *data, size_t length) {
        const bool *data64 = data;
        size_t i = 0;
        unsigned long long unpack;
        for (; i < length / 8; ++i) {
            unsigned long long mask = 0x0101010101010101ULL;
            unsigned long long tmp = 0;
            memcpy(&unpack, data64, sizeof(unpack));
            data64 += sizeof(unpack);
#if defined(__BMI2__)
            tmp = _pext_u64(unpack, mask);
#else
            // https://github.com/Forceflow/libmorton/issues/6
            for (unsigned long long bb = 1; mask != 0; bb += bb) {
                if (unpack & mask & -mask) {
                    tmp |= bb;
                }
                mask &= (mask - 1);
            }
#endif
            send_data(&tmp, 1);
        }
        if (8 * i != length)
            send_data(data + 8 * i, length - 8 * i);
    }
    void recv_bool_aligned(bool *data, size_t length) {
        bool *data64 = data;
        size_t i = 0;
        unsigned long long unpack;
        for (; i < length / 8; ++i) {
            unsigned long long mask = 0x0101010101010101ULL;
            unsigned long long tmp = 0;
            recv_data(&tmp, 1);
#if defined(__BMI2__)
            unpack = _pdep_u64(tmp, mask);
#else
            unpack = 0;
            for (unsigned long long bb = 1; mask != 0; bb += bb) {
                if (tmp & bb) {
                    unpack |= mask & (-mask);
                }
                mask &= (mask - 1);
            }
#endif
            memcpy(data64, &unpack, sizeof(unpack));
            data64 += sizeof(unpack);
        }
        if (8 * i != length)
            recv_data(data + 8 * i, length - 8 * i);
    }

private:
    T &derived() {
        return *static_cast<T *>(this);
    }
};

class FileIO : public IOChannel<FileIO> {
public:
    uint64_t bytes_sent = 0;
    int mysocket = -1;
    FILE *stream = nullptr;
    char *buffer = nullptr;
    FileIO(const char *file, bool read);
    ~FileIO();
    void send_data_internal(const void *data, int len);
    void recv_data_internal(void *data, int len);

    inline void flush() {
        fflush(stream);
    }

    inline void reset() {
        rewind(stream);
    }
};

class SubChannel {
public:
    int sock;
    FILE *stream = nullptr;
    char *buf = nullptr;
    int ptr;
    char *stream_buf = nullptr;
    uint64_t counter = 0;
    uint64_t flushes = 0;
    SubChannel(int sock);
    ~SubChannel();
};

class SenderSubChannel : public SubChannel {
public:
    SenderSubChannel(int sock);
    void flush();
    void send_data(const void *data, int len);
    void send_data_raw(const void *data, int len);
};

class RecverSubChannel : public SubChannel {
public:
    RecverSubChannel(int sock);
    void flush();
    void recv_data(void *data, int len);
    void recv_data_raw(void *data, int len);
};

class HighSpeedNetIO : public IOChannel<HighSpeedNetIO> {
public:
    bool is_server, quiet;
    int send_sock = 0;
    int recv_sock = 0;
    int FSM = 0;
    SenderSubChannel *schannel;
    RecverSubChannel *rchannel;
    HighSpeedNetIO(const char *address, int send_port, int recv_port, bool quiet = true);
    ~HighSpeedNetIO();
    int server_listen(int port);
    int client_connect(const char *address, int port);
    void set_delay_opt(int sock, bool enable_nodelay);
    void flush();
    void send_data_internal(const void *data, int len);
    void recv_data_internal(void *data, int len);
};

class MemIO : public IOChannel<MemIO> {
public:
    char *buffer = nullptr;
    int64_t size = 0;
    int64_t read_pos = 0;
    int64_t cap;
    MemIO(int64_t _cap = 1024 * 1024);
    ~MemIO();
    void load_from_file(FileIO *fio, int64_t size);
    void send_data_internal(const void *data, int64_t len);
    void recv_data_internal(void *data, int64_t len);

    inline void clear() {
        size = 0;
    }
};

class NetIO : public IOChannel<NetIO> {
public:
    bool is_server;
    int mysocket = -1;
    int consocket = -1;
    FILE *stream = nullptr;
    char *buffer = nullptr;
    bool has_sent = false;
    string addr;
    int port;
    NetIO(const char *address, int port, bool quiet = false);
    ~NetIO();
    void sync();
    void send_data_internal(const void *data, size_t len);
    void recv_data_internal(void *data, size_t len);

    inline void set_nodelay() {
        const int one = 1;
        setsockopt(consocket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    }

    inline void set_delay() {
        const int zero = 0;
        setsockopt(consocket, IPPROTO_TCP, TCP_NODELAY, &zero, sizeof(zero));
    }

    inline void flush() {
        fflush(stream);
    }
};
#endif
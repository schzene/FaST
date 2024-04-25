#ifndef FAST_IO_H__
#define FAST_IO_H__
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
using std::string;

#include <arpa/inet.h>
#include <emmintrin.h>
#include <immintrin.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef __m128i block128;
typedef __m256i block256;

#include <config.h>

const static int NETWORK_BUFFER_SIZE = 1024 * 16; // Should change depending on the network

template <typename T>
class IOChannel
{
public:
    void send_data(const void *data, int nbyte)
    {
        derived().send_data(data, nbyte);
    }

    void recv_data(void *data, int nbyte)
    {
        derived().recv_data(data, nbyte);
    }

    void send_block(const block128 *data, int nblock)
    {
        send_data(data, nblock * sizeof(block128));
    }

    void send_block(const block256 *data, int nblock)
    {
        send_data(data, nblock * sizeof(block256));
    }

    void recv_block(block128 *data, int nblock)
    {
        recv_data(data, nblock * sizeof(block128));
    }

private:
    T &derived()
    {
        return *static_cast<T *>(this);
    }
};

enum class LastCall
{
    None,
    Send,
    Recv
};

class NetIO : public IOChannel<NetIO>
{
public:
    bool is_server;
    int mysocket = -1;
    int consocket = -1;
    FILE *stream = nullptr;
    char *buffer = nullptr;
    bool has_sent = false;
    string addr;
    int port;
    uint64_t counter = 0;
    uint64_t num_rounds = 0;
    bool FBF_mode;
    LastCall last_call = LastCall::None;

    NetIO(const char *address, int port, bool full_buffer = false, bool quiet = false);
    ~NetIO();
    void sync();
    void send_data(const void *data, int len);
    void recv_data(void *data, int len);

    inline void set_FBF()
    {
        flush();
        setvbuf(stream, buffer, _IOFBF, NETWORK_BUFFER_SIZE);
    }

    inline void set_NBF()
    {
        flush();
        setvbuf(stream, buffer, _IONBF, NETWORK_BUFFER_SIZE);
    }

    inline void set_nodelay()
    {
        const int one = 1;
        setsockopt(consocket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    }

    inline void set_delay()
    {
        const int zero = 0;
        setsockopt(consocket, IPPROTO_TCP, TCP_NODELAY, &zero, sizeof(zero));
    }

    inline void flush()
    {
        fflush(stream);
    }
};

class IOPack
{
public:
    NetIO *io;
    NetIO *io_rev;

    IOPack(int party, std::string address = "127.0.0.1");
    ~IOPack();
    void send_data(const void *data, int len);
    void recv_data(void *data, int len);

    inline uint64_t get_rounds()
    {
        // no need to count io_rev->num_rounds
        // as io_rev is only used in parallelwith io
        return io->num_rounds;
    }

    inline uint64_t get_comm()
    {
        return io->counter + io_rev->counter;
    }
};
#endif
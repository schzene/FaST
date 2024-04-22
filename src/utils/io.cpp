#include "io.h"

FileIO::FileIO(const char *file, bool read) {
    if (read) {
        stream = fopen(file, "rb+");
    } else {
        stream = fopen(file, "wb+");
    }

    buffer = new char[FILE_BUFFER_SIZE];
    memset(buffer, 0, FILE_BUFFER_SIZE);
    setvbuf(stream, buffer, _IOFBF, FILE_BUFFER_SIZE);
}

FileIO::~FileIO() {
    fflush(stream);
    fclose(stream);
    delete[] buffer;
}

void FileIO::send_data_internal(const void *data, int len) {
    bytes_sent += len;
    int sent = 0;
    while (sent < len) {
        int res = fwrite(sent + (char *)data, 1, len - sent, stream);
        if (res >= 0) {
            sent += res;
        } else {
            fprintf(stderr, "error: file_send_data %d\n", res);
        }
    }
}

void FileIO::recv_data_internal(void *data, int len) {
    int sent = 0;
    while (sent < len) {
        int res = fread(sent + (char *)data, 1, len - sent, stream);
        if (res >= 0) {
            sent += res;
        } else {
            fprintf(stderr, "error: file_send_data %d\n", res);
        }
    }
}

SubChannel::SubChannel(int sock) : sock(sock) {
    stream_buf = new char[NETWORK_BUFFER_SIZE];
    buf = new char[NETWORK_BUFFER_SIZE2];
    stream = fdopen(sock, "wb+");
    memset(stream_buf, 0, NETWORK_BUFFER_SIZE);
    setvbuf(stream, stream_buf, _IOFBF, NETWORK_BUFFER_SIZE);
}

SubChannel::~SubChannel() {
    fclose(stream);
    delete[] stream_buf;
    delete[] buf;
}

SenderSubChannel::SenderSubChannel(int sock) : SubChannel(sock) {
    ptr = 0;
}

void SenderSubChannel::flush() {
    flushes++;
    send_data_raw(buf, ptr);
    if (counter % NETWORK_BUFFER_SIZE2 != 0) {
        send_data_raw(buf + ptr, NETWORK_BUFFER_SIZE2 - counter % NETWORK_BUFFER_SIZE2);
    }
    fflush(stream);
    ptr = 0;
}

void SenderSubChannel::send_data(const void *data, int len) {
    if (len <= NETWORK_BUFFER_SIZE2 - ptr) {
        memcpy(buf + ptr, data, len);
        ptr += len;
    } else {
        send_data_raw(buf, ptr);
        send_data_raw(data, len);
        ptr = 0;
    }
}

void SenderSubChannel::send_data_raw(const void *data, int len) {
    counter += len;
    int sent = 0;
    while (sent < len) {
        int res = fwrite(sent + (char *)data, 1, len - sent, stream);
        if (res >= 0) {
            sent += res;
        } else {
            fprintf(stderr, "error: file_send_data %d\n", res);
        }
    }
}

RecverSubChannel::RecverSubChannel(int sock) : SubChannel(sock) {
    ptr = NETWORK_BUFFER_SIZE2;
}

void RecverSubChannel::flush() {
    flushes++;
    ptr = NETWORK_BUFFER_SIZE2;
}

void RecverSubChannel::recv_data(void *data, int len) {
    if (len <= NETWORK_BUFFER_SIZE2 - ptr) {
        memcpy(data, buf + ptr, len);
        ptr += len;
    } else {
        int remain = len;
        memcpy(data, buf + ptr, NETWORK_BUFFER_SIZE2 - ptr);
        remain -= NETWORK_BUFFER_SIZE2 - ptr;

        while (true) {
            recv_data_raw(buf, NETWORK_BUFFER_SIZE2);
            if (remain <= NETWORK_BUFFER_SIZE2) {
                memcpy(len - remain + (char *)data, buf, remain);
                ptr = remain;
                break;
            } else {
                memcpy(len - remain + (char *)data, buf, NETWORK_BUFFER_SIZE2);
                remain -= NETWORK_BUFFER_SIZE2;
            }
        }
    }
}

void RecverSubChannel::recv_data_raw(void *data, int len) {
    counter += len;
    int sent = 0;
    while (sent < len) {
        int res = fread(sent + (char *)data, 1, len - sent, stream);
        if (res >= 0) {
            sent += res;
        } else {
            fprintf(stderr, "error: file_send_data %d\n", res);
        }
    }
}

HighSpeedNetIO::HighSpeedNetIO(const char *address, int send_port, int recv_port, bool quiet) : quiet(quiet) {
    if (send_port < 0 || send_port > 65535) {
        throw std::runtime_error("Invalid send port number!");
    }
    if (recv_port < 0 || recv_port > 65535) {
        throw std::runtime_error("Invalid receive port number!");
    }

    is_server = (address == nullptr);
    if (is_server) {
        recv_sock = server_listen(send_port);
        usleep(2000);
        send_sock = server_listen(recv_port);
    } else {
        send_sock = client_connect(address, send_port);
        recv_sock = client_connect(address, recv_port);
    }
    FSM = 0;
    set_delay_opt(send_sock, true);
    set_delay_opt(recv_sock, true);
    schannel = new SenderSubChannel(send_sock);
    rchannel = new RecverSubChannel(recv_sock);
    if (not quiet) {
        std::cout << "connected\n";
    }
}

HighSpeedNetIO::~HighSpeedNetIO() {
    flush();
    if (not quiet) {
        std::cout << "Data Sent: \t" << schannel->counter << "\n";
        std::cout << "Data Received: \t" << rchannel->counter << "\n";
        std::cout << "Flushes:\t" << schannel->flushes << "\t" << rchannel->flushes << "\n";
    }
    delete schannel;
    delete rchannel;
    close(send_sock);
    close(recv_sock);
}

int HighSpeedNetIO::server_listen(int port) {
    int mysocket;
    struct sockaddr_in dest;
    struct sockaddr_in serv;
    socklen_t socksize = sizeof(struct sockaddr_in);
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY); /* set our address to any interface */
    serv.sin_port = htons(port);              /* set the server port number */
    mysocket = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    setsockopt(mysocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));
    if (bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr)) < 0) {
        perror("error: bind");
        exit(1);
    }
    if (listen(mysocket, 1) < 0) {
        perror("error: listen");
        exit(1);
    }
    int sock = accept(mysocket, (struct sockaddr *)&dest, &socksize);
    close(mysocket);
    return sock;
}

int HighSpeedNetIO::client_connect(const char *address, int port) {
    int sock;
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(address);
    dest.sin_port = htons(port);

    while (1) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(sock, (struct sockaddr *)&dest, sizeof(struct sockaddr)) == 0) {
            break;
        }

        close(sock);
        usleep(1000);
    }
    return sock;
}

void HighSpeedNetIO::set_delay_opt(int sock, bool enable_nodelay) {
    if (enable_nodelay) {
        const int one = 1;
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    } else {
        const int zero = 0;
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &zero, sizeof(zero));
    }
}

void HighSpeedNetIO::flush() {
    if (is_server) {
        schannel->flush();
        rchannel->flush();
    } else {
        rchannel->flush();
        schannel->flush();
    }
    FSM = 0;
}

void HighSpeedNetIO::send_data_internal(const void *data, int len) {
    if (FSM == 1) {
        rchannel->flush();
    }
    schannel->send_data(data, len);
    FSM = 2;
}

void HighSpeedNetIO::recv_data_internal(void *data, int len) {
    if (FSM == 2) {
        schannel->flush();
    }
    rchannel->recv_data(data, len);
    FSM = 1;
}

MemIO::MemIO(int64_t _cap) {
    this->cap = _cap;
    buffer = new char[cap];
    size = 0;
}

MemIO::~MemIO() {
    if (buffer != nullptr) {
        delete[] buffer;
    }
}

void MemIO::load_from_file(FileIO *fio, int64_t size) {
    delete[] buffer;
    buffer = new char[size];
    this->cap = size;
    this->read_pos = 0;
    this->size = size;
    fio->recv_data_internal(buffer, size);
}

void MemIO::send_data_internal(const void *data, int64_t len) {
    if (size + len >= cap) {
        char *new_buffer = new char[2 * (cap + len)];
        memcpy(new_buffer, buffer, size);
        delete[] buffer;
        buffer = new_buffer;
        cap = 2 * (cap + len);
    }
    memcpy(buffer + size, data, len);
    size += len;
}

void MemIO::recv_data_internal(void *data, int64_t len) {
    if (read_pos + len <= size) {
        memcpy(data, buffer + read_pos, len);
        read_pos += len;
    } else {
        fprintf(stderr, "error: mem_recv_data\n");
    }
}

NetIO::NetIO(const char *address, int port, bool quiet) {
    if (port < 0 || port > 65535) {
        throw std::runtime_error("Invalid port number!");
    }

    this->port = port;
    is_server = (address == nullptr);
    if (address == nullptr) {
        struct sockaddr_in dest;
        struct sockaddr_in serv;
        socklen_t socksize = sizeof(struct sockaddr_in);
        memset(&serv, 0, sizeof(serv));
        serv.sin_family = AF_INET;
        serv.sin_addr.s_addr = htonl(INADDR_ANY); /* set our address to any interface */
        serv.sin_port = htons(port);              /* set the server port number */
        mysocket = socket(AF_INET, SOCK_STREAM, 0);
        int reuse = 1;
        setsockopt(mysocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));
        if (bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr)) < 0) {
            perror("error: bind");
            exit(1);
        }
        if (listen(mysocket, 1) < 0) {
            perror("error: listen");
            exit(1);
        }
        consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
        close(mysocket);
    } else {
        addr = string(address);

        struct sockaddr_in dest;
        memset(&dest, 0, sizeof(dest));
        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = inet_addr(address);
        dest.sin_port = htons(port);

        while (1) {
            consocket = socket(AF_INET, SOCK_STREAM, 0);

            if (connect(consocket, (struct sockaddr *)&dest, sizeof(struct sockaddr)) == 0) {
                break;
            }

            close(consocket);
            usleep(1000);
        }
    }
    set_nodelay();
    stream = fdopen(consocket, "wb+");
    buffer = new char[NETWORK_BUFFER_SIZE];
    memset(buffer, 0, NETWORK_BUFFER_SIZE);
    setvbuf(stream, buffer, _IOFBF, NETWORK_BUFFER_SIZE);
    if (!quiet) {
        std::cout << "connected\n";
    }
}

NetIO::~NetIO() {
    flush();
    fclose(stream);
    delete[] buffer;
}

void NetIO::sync() {
    int tmp = 0;
    if (is_server) {
        send_data_internal(&tmp, 1);
        recv_data_internal(&tmp, 1);
    } else {
        recv_data_internal(&tmp, 1);
        send_data_internal(&tmp, 1);
        flush();
    }
}

void NetIO::send_data_internal(const void *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        size_t res = fwrite(sent + (char *)data, 1, len - sent, stream);
        if (res > 0)
            sent += res;
        else
            error("net_send_data\n");
    }
    has_sent = true;
}

void NetIO::recv_data_internal(void  * data, size_t len) {
		if(has_sent)
			fflush(stream);
		has_sent = false;
		size_t sent = 0;
		while(sent < len) {
			size_t res = fread(sent + (char*)data, 1, len - sent, stream);
			if (res > 0)
				sent += res;
			else
				error("net_recv_data\n");
		}
	}
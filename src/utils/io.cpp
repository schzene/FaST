#include "io.h"

NetIO::NetIO(const char *address, int port, bool full_buffer, bool quiet) {
    this->port = port;
    is_server = (address == nullptr);
    if (address == nullptr) {
        struct sockaddr_in dest;
        struct sockaddr_in serv;
        socklen_t socksize = sizeof(struct sockaddr_in);
        memset(&serv, 0, sizeof(serv));
        serv.sin_family = AF_INET;
        serv.sin_addr.s_addr =
            htonl(INADDR_ANY);       /* set our address to any interface */
        serv.sin_port = htons(port); /* set the server port number */
        mysocket = socket(AF_INET, SOCK_STREAM, 0);
        int reuse = 1;
        setsockopt(mysocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse,
                   sizeof(reuse));
        if (::bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr)) <
            0) {
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

            if (connect(consocket, (struct sockaddr *)&dest,
                        sizeof(struct sockaddr)) == 0) {
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
    if (full_buffer) {
        setvbuf(stream, buffer, _IOFBF, NETWORK_BUFFER_SIZE);
    } else {
        setvbuf(stream, buffer, _IONBF, NETWORK_BUFFER_SIZE);
    }
    this->FBF_mode = full_buffer;
    if (!quiet)
        std::cout << "connected\n";
}

NetIO::~NetIO() {
    fflush(stream);
    close(consocket);
    delete[] buffer;
}

void NetIO::sync() {
    int tmp = 0;
    if (is_server) {
        send_data(&tmp, 1);
        recv_data(&tmp, 1);
    } else {
        recv_data(&tmp, 1);
        send_data(&tmp, 1);
        flush();
    }
}

void NetIO::send_data(const void *data, int len) {
    if (last_call != LastCall::Send) {
        num_rounds++;
        last_call = LastCall::Send;
    }
    counter += len;
    int sent = 0;
    while (sent < len) {
        int res = fwrite(sent + (char *)data, 1, len - sent, stream);
        if (res >= 0) {
            sent += res;
        }
        else {
            fprintf(stderr, "error: net_send_data %d\n", res);
        }
    }
    has_sent = true;
}

void NetIO::recv_data(void *data, int len) {
    if (last_call != LastCall::Recv) {
        num_rounds++;
        last_call = LastCall::Recv;
    }
    if (has_sent) {
        fflush(stream);
    }
    has_sent = false;
    int sent = 0;
    while (sent < len) {
        int res = fread(sent + (char *)data, 1, len - sent, stream);
        if (res >= 0) {
            sent += res;
        }
        else {
            fprintf(stderr, "error: net_send_data %d\n", res);
        }
    }
}

IOPack::IOPack(int party, int port, std::string) {
    this->party = party;
    this->port = port;
    this->io = new NetIO(party == 1 ? nullptr : address.c_str(), port, false, false);
    this->io_rev = new NetIO(party == 1 ? nullptr : address.c_str(),
                             port + REV_PORT_OFFSET, false, true);
}

IOPack::~IOPack() {
    delete io;
    delete io_rev;
}
#include <utils.h>

int main(int argc, const char *argv[]) {
    int party = argc > 1 ? 1 : 2;
    IOPack* io_pack = new IOPack(party);
    if (party == ALICE) {
        std::vector<double> data = {1,2,3,4};
        send_mat(io_pack, &data);
    } else {
        std::vector<double> data(3);
        recv_mat(io_pack, &data);
        std::cout << data[2] << "\n";
    }
    if (party == ALICE) {
        size_t data;
        io_pack->recv_data(&data, sizeof(size_t));
        std::cout << data << "\n";
    } else {
        size_t data = 54321;
        io_pack->send_data(&data, sizeof(size_t));
    }
    delete io_pack;
}
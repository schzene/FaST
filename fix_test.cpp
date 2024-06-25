#include <iostream>
#include <random>

using std::cout;

int main() {
    const int BW = 30;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(0, 0.1);
    double r = dist(gen), div_r = 1 / r, x = dist(gen);
    const uint64_t mask = 1ULL << BW;
    uint64_t fix_r = (uint64_t) (mask * r);
    uint64_t fix_div_r = (uint64_t) (mask * div_r);
    uint64_t fix_x = (uint64_t) (mask * x * r);
    fix_x *= fix_div_r;
    fix_x >>= BW;
    std::cout << "error: " << x - double(fix_x) / double(mask) << "\n";
}
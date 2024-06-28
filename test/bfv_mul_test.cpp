#include <utils.h>

#include <iostream>
using std::ostream;

class Complex {
public:
    double real;
    double i, j, k;

    Complex() {}
    Complex(double _real, double _i = 0, double _j = 0, double _k = 0) : real(_real), i(_i), j(_j), k(_k) {}

    inline Complex operator+(Complex &other) const {
        return Complex(real + other.real, i + other.i, j + other.j, k + other.k);
    }

    inline Complex operator+(double other) const { return Complex(real + other, i, j, k); }

    inline Complex operator-(Complex &other) const {
        return Complex(real - other.real, i - other.i, j - other.j, k - other.k);
    }

    inline Complex operator-(double other) const { return Complex(real - other, i, j, k); }

    inline Complex operator*(Complex &other) const {
        return Complex(real * other.real - i * other.i - j * other.j - k * other.k,
                       real * other.i + i * other.real + j * other.k - k * other.j,
                       real * other.j - i * other.k + j * other.real + k * other.i,
                       real * other.k + i * other.j - j * other.i + k * other.real);
    }

    inline Complex operator*(double other) const { return Complex(real * other, i * other, j * other, k * other); }

    inline Complex operator/(double other) const { return Complex(real / other, i / other, j / other, k / other); }

    inline Complex conjugate() const { return Complex(real, -i, -j, -k); }

    inline double dot(Complex &other) const { return real * other.real + i * other.i + j * other.j + k * other.k; }

    inline Complex outer(Complex &other) const {
        return Complex(0, i * other.real - real * other.i + j * other.k - k * other.j,
                       j * other.real - real * other.j + k * other.i - i * other.k,
                       k * other.real - real * other.k + i * other.j - j * other.i);
    }

    inline Complex even(Complex &other) const {
        return Complex(real * other.real - i * other.i - j * other.j - k * other.k, real * other.i + i * other.real,
                       real * other.j + j * other.real, real * other.k + k * other.real);
    }

    inline Complex cross(Complex &other) const {
        return Complex(0, j * other.k - k * other.j, k * other.i - i * other.k, i * other.j - j * other.i);
    }

    inline Complex transpose() const {
        double square_modulo = real * real + i * i + j * j + k * k;
        return Complex(real / square_modulo, -i / square_modulo, -j / square_modulo, -k / square_modulo);
    }

    inline double modulo() const { return sqrt(real * real + i * i + j * j + k * k); }

    inline double scalar() const { return real; }

    inline Complex vector() const { return Complex(i, j, k); }

    inline Complex sgn() const {
        double modulo = sqrt(real * real + i * i + j * j + k * k);
        return Complex(real / modulo, i / modulo, j / modulo, k / modulo);
    }

    inline double arg() const { return acos(real / sqrt(real * real + i * i + j * j + k * k)); }

    friend ostream &operator<<(ostream &os, const Complex &c);
};

ostream &operator<<(ostream &os, const Complex &c) {
    os << c.real;
    if (abs(c.i) > 1e-16) {
        if (c.i >= 0) {
            os << "+";
        }
        os << c.i << "i";
    }
    if (abs(c.j) > 1e-16) {
        if (c.j >= 0) {
            os << "+";
        }
        os << c.j << "j";
    }
    if (abs(c.k) > 1e-16) {
        if (c.k >= 0) {
            os << "+";
        }
        os << c.k << "k";
    }
    return os;
}

int main() {
    vector<uint64_t> a = {41235};
    vector<uint64_t> b = {13020}; // change it to 13021
    uint64_t mask = 1ULL << 16;
    std::cout << "true result: " << double(a[0] * b[0]) / double(mask * mask) << "\n";
    // std::cout << "true result: " << a[0] * b[0] << "\n";

    BFVParm *bfv_parm = new BFVParm(8192, {54, 54, 55, 55}, default_prime_mod.at(29));
    BFVKey *party = new BFVKey(1, bfv_parm);
    Plaintext pta, ptb, ptaa;
    bfv_parm->encoder->encode(a, pta), bfv_parm->encoder->encode(b, ptb);
    Ciphertext cta;
    party->encryptor->encrypt(pta, cta);
    bfv_parm->evaluator->multiply_plain_inplace(cta, ptb);
    party->decryptor->decrypt(cta, ptaa);
    vector<uint64_t> res(1);
    bfv_parm->encoder->decode(ptaa, res);
    std::cout << "result: " << double(res[0]) / double(mask * mask) << "\n";
    // std::cout << "result: " << res[0] << "\n";
}
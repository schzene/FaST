
#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <sstream>
#include <vector>
#define TEST
using namespace std;

int64_t neg_mod(int64_t val, int64_t mod) {
    return ((val % mod) + mod) % mod;
}

vector<vector<uint64_t>> read_data(const string &filename) {
    ifstream input_file(filename);
    vector<vector<uint64_t>> data;

    if (!input_file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return data;
    }

    string line;

    while (getline(input_file, line)) {
        vector<uint64_t> row;
        istringstream line_stream(line);
        string cell;
        cout << "start!" << endl;

        while (getline(line_stream, cell, ',')) {
            row.push_back(stoll(cell));
        }

        data.push_back(row);
    }

    input_file.close();
    return data;
}

int main() {

    string path = "data.txt";
    vector<vector<uint64_t>> input_plain = read_data(path);

    uint64_t h1[2 * 6] = {0};

    int32_t bitlength = 32;
    uint64_t prime_mod = 4293918721; // 32 bit

    for (int i = 0; i < 2; i++) {

        for (int j = 0; j <= 6; j++) {
            cout << "plain: " << input_plain[i][j] << " ";
            h1[i * 2 + j] = neg_mod(((int64_t)input_plain[i][j]), (int64_t)prime_mod);
            cout << "h1_neg_mod: " << h1[i * 2 + j] << " ";
        }
        cout << "\n";
    }
}

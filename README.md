# FaST: Fast and Secure Transformer

FaST is Fast and Secure Transformer model implement with C++, based on bert.

# Build

Build FaST is simple, just run:

```bash
mkdir build && cd build
cmake ../
make
```

if you want to turn off the test sample, run:

```bash
mkdir build && cd build
cmake ../ -DFaST_TEST=OFF
make
```

# Acknowledgments

This repository includes code from the following external repositories:

[Microsoft/SEAL](https://github.com/microsoft/SEAL) for cryptographic tools.

[emp-toolkit/emp-tool](https://github.com/emp-toolkit/emp-tool) for Network IO
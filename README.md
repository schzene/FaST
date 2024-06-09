# FaST: Fast and Secure Transformer

FaST is Fast and Secure Transformer model implement with C++, based on bert.

# Build

You can build FaST using following commands:

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

[Microsoft/EzPC/SCI](https://github.com/mpc-msri/EzPC/tree/master/SCI) for Network IO and fixed-point basic operation.

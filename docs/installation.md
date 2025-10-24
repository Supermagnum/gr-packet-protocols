# Installation Guide

This guide covers the installation and setup of the gr-packet-protocols GNU Radio Out-of-Tree (OOT) module.

## Prerequisites

### System Requirements
- **Operating System**: Linux (Ubuntu 20.04+ recommended)
- **GNU Radio**: Version 3.8+ or 3.10+
- **CMake**: Version 3.16+
- **C++ Compiler**: GCC 7+ or Clang 6+
- **Python**: Version 3.6+

### Required Dependencies
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y \
    gnuradio-dev \
    gnuradio \
    cmake \
    build-essential \
    pkg-config \
    python3-dev \
    python3-numpy \
    python3-scipy \
    python3-matplotlib \
    libboost-all-dev \
    liblog4cpp5-dev \
    libzmq3-dev \
    libfftw3-dev \
    libgsl-dev \
    libsndfile1-dev \
    libasound2-dev \
    libpulse-dev \
    libjack-jackd2-dev \
    libgmp-dev \
    libmpfr-dev \
    libmpc-dev \
    libgsl-dev \
    libgsl0-dev \
    libgsl0ldbl \
    libgslcblas0 \
    libgslcblas0-dev
```

### Optional Dependencies
```bash
# For fuzzing and security testing
sudo apt install -y \
    afl++ \
    clang \
    llvm \
    cppcheck \
    flawfinder \
    semgrep

# For documentation generation
sudo apt install -y \
    doxygen \
    graphviz \
    texlive-latex-extra
```

## Installation Methods

### Method 1: From Source (Recommended)

1. **Clone the repository**:
```bash
git clone https://github.com/Supermagnum/gr-packet-protocols.git
cd gr-packet-protocols
```

2. **Create build directory**:
```bash
mkdir build
cd build
```

3. **Configure with CMake**:
```bash
cmake ..
```

4. **Build the module**:
```bash
make -j$(nproc)
```

5. **Install the module**:
```bash
sudo make install
```

6. **Update library cache**:
```bash
sudo ldconfig
```

### Method 2: Using gr_modtool (Development)

1. **Create new OOT module**:
```bash
gr_modtool newmod packet_protocols
cd gr-packet-protocols
```

2. **Add blocks**:
```bash
gr_modtool add -t general -l ax25 ax25_encoder
gr_modtool add -t general -l ax25 ax25_decoder
gr_modtool add -t general -l fx25 fx25_encoder
gr_modtool add -t general -l fx25 fx25_decoder
gr_modtool add -t general -l il2p il2p_encoder
gr_modtool add -t general -l il2p il2p_decoder
gr_modtool add -t general -l ax25 kiss_tnc
```

3. **Build and install**:
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

## Verification

### Test Installation
```bash
# Test GNU Radio installation
gnuradio-companion --version

# Test Python bindings
python3 -c "import gnuradio.packet_protocols; print('Success!')"

# Test C++ compilation
g++ -I/usr/include/gnuradio -I/usr/include/gnuradio/packet_protocols \
    -o test_compile test_file.cpp -lgnuradio-packet_protocols
```

### Run Examples
```bash
# Test AX.25 example
gnuradio-companion examples/ax25_kiss_example.grc

# Test FX.25 example
gnuradio-companion examples/fx25_fec_example.grc

# Test IL2P example
gnuradio-companion examples/il2p_example.grc
```

## Configuration

### Environment Variables
```bash
# Add to ~/.bashrc or ~/.profile
export GR_PACKET_PROTOCOLS_ROOT="/usr/local"
export PYTHONPATH="$PYTHONPATH:/usr/local/lib/python3/dist-packages"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib"
```

### GNU Radio Configuration
```bash
# Update GNU Radio block registry
gnuradio-config-info --version
gnuradio-config-info --prefix
```

## Troubleshooting

### Common Issues

1. **Import Error**: `ModuleNotFoundError: No module named 'gnuradio.packet_protocols'`
   - **Solution**: Check PYTHONPATH and ensure module is installed correctly
   - **Fix**: `export PYTHONPATH="$PYTHONPATH:/usr/local/lib/python3/dist-packages"`

2. **Library Not Found**: `error while loading shared libraries`
   - **Solution**: Update library cache and check LD_LIBRARY_PATH
   - **Fix**: `sudo ldconfig && export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib"`

3. **CMake Configuration Failed**
   - **Solution**: Check GNU Radio installation and CMake version
   - **Fix**: `sudo apt install gnuradio-dev cmake`

4. **Compilation Errors**
   - **Solution**: Check C++ compiler and dependencies
   - **Fix**: `sudo apt install build-essential g++`

### Debug Information
```bash
# Check GNU Radio installation
gnuradio-config-info --version
gnuradio-config-info --prefix
gnuradio-config-info --cflags
gnuradio-config-info --libs

# Check Python bindings
python3 -c "import gnuradio; print(gnuradio.__version__)"
python3 -c "import gnuradio.packet_protocols; print('Module loaded')"

# Check library paths
ldconfig -p | grep gnuradio
ldd /usr/local/lib/libgnuradio-packet_protocols.so
```

## Uninstallation

```bash
# Remove installed files
sudo rm -rf /usr/local/lib/libgnuradio-packet_protocols*
sudo rm -rf /usr/local/include/gnuradio/packet_protocols/
sudo rm -rf /usr/local/lib/python3/dist-packages/gnuradio/packet_protocols/
sudo rm -rf /usr/local/share/gnuradio/grc/blocks/packet_protocols/

# Update library cache
sudo ldconfig
```

## Next Steps

After successful installation, proceed to:
- [User Guide](user-guide/getting-started.md) - Learn how to use the protocols
- [Examples](examples/) - Try the provided examples
- [API Reference](api/) - Explore the complete API documentation

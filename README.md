# gr-packet-protocols

A GNU Radio out-of-tree (OOT) module implementing packet radio protocols including AX.25, FX.25, and IL2P.

This is offering complete packet radio protocol support for GNU Radio applications.

## Features

### AX.25 Protocol
- Complete AX.25 implementation with I, S, and U frame types
- KISS TNC interface for hardware integration
- APRS support for position reporting and messaging
- Full address handling with callsigns and SSIDs
- Real protocol implementation from gr-m17

### FX.25 Support
- Forward Error Correction (FEC) for AX.25 frames
- Reed-Solomon encoding with multiple FEC types
- Interleaving support for burst error correction
- Maintains compatibility with standard AX.25


### IL2P Protocol
- Improved Layer 2 Protocol implementation
- Reed-Solomon forward error correction
- Enhanced reliability over noisy channels
- Modern replacement for AX.25


## Installation

### Prerequisites
- GNU Radio 3.10.12.0 or later (tested with 3.10.12.0)
- CMake 3.16 or later
- C++17 compatible compiler
- Python 3 (for bindings)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/your-username/gr-packet-protocols.git
cd gr-packet-protocols

# Create build directory
mkdir build
cd build

# Configure with CMake
# For system-wide installation (recommended):
cmake .. -DCMAKE_INSTALL_PREFIX=/usr

# Or for local installation:
# cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local

# Build the module
make -j$(nproc)

# Install the module
sudo make install
sudo ldconfig
```

**Note for GNU Radio 3.10+**: This module uses YAML block definitions (`.block.yml`) which are compatible with GNU Radio 3.10.12.0 and later. The installation paths are automatically configured to match your GNU Radio installation.

### Dependencies

The following packages are required:

```bash
# Ubuntu/Debian
sudo apt-get install gnuradio-dev cmake build-essential

# Fedora/RHEL
sudo dnf install gnuradio-devel cmake gcc-c++

# macOS (with Homebrew)
brew install gnuradio cmake
```

## Usage

### GNU Radio Companion

The module provides blocks in the "Packet Protocols" category:

- **AX.25 Encoder**: Encodes data into AX.25 frames
- **AX.25 Decoder**: Decodes AX.25 frames
- **KISS TNC**: Interface to KISS-compatible TNCs
- **FX.25 Encoder**: Encodes with forward error correction
- **FX.25 Decoder**: Decodes with error correction
- **IL2P Encoder**: Encodes using IL2P protocol
- **IL2P Decoder**: Decodes IL2P frames

### Python API

```python
import gnuradio.packet_protocols as pp

# Create AX.25 encoder
encoder = pp.ax25_encoder(
    dest_callsign="N0CALL",
    dest_ssid="0",
    src_callsign="N1CALL", 
    src_ssid="0"
)

# Create KISS TNC interface
tnc = pp.kiss_tnc(
    device="/dev/ttyUSB0",
    baud_rate=9600
)
```

## Examples

Example flowgraphs are provided in the `examples/` directory:

- `ax25_kiss_example.grc`: AX.25 with KISS TNC interface
- `fx25_fec_example.grc`: FX.25 forward error correction
- `il2p_example.grc`: IL2P protocol demonstration

## Protocol Details

### AX.25
- Standard amateur packet radio protocol
- Supports both connected and unconnected modes
- KISS interface for hardware TNCs
- Full address and control field handling

### FX.25
- Extends AX.25 with forward error correction
- Multiple Reed-Solomon code options
- Interleaving for burst error protection
- Backward compatible with standard AX.25

### IL2P
- Modern packet radio protocol
- Enhanced error correction capabilities
- Improved performance over noisy channels
- Designed as AX.25 replacement

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Based on the gr-m17 project structure
- Inspired by Dire Wolf AX.25 implementation
- GNU Radio community for excellent framework

## Security Testing

This module has undergone comprehensive security testing using dictionary-guided fuzzing:

- **Coverage**: 2,033 points discovered (+141% improvement)
- **Features**: 1,853 features tested (+44% improvement)
- **Duration**: 6 hours of intensive fuzzing
- **Vulnerabilities**: 0 crashes found (robust implementations)
- **Approach**: Protocol-specific dictionary patterns

For detailed results, see [Fuzzing Results Report](fuzzing-results.md).

## Support

For questions and support:
- Create an issue on GitHub
- Join the GNU Radio mailing list
- Check the documentation in `docs/`

## Uninstallation

To uninstall the module:

```bash
cd build
sudo make uninstall
sudo ldconfig
```

This will remove all installed files. If you don't have the build directory, see the [Installation Guide](docs/installation.md) for manual removal instructions.

## Changelog

### Version 1.0.0
- Initial release
- AX.25 protocol implementation
- FX.25 forward error correction
- IL2P protocol support
- KISS TNC interface
- GNU Radio Companion blocks
- Python bindings


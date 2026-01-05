# gr-packet-protocols

**IMPORTANT NOTICE**: This is AI-generated code. The developer has a neurological condition that makes it impossible to use and learn traditional programming. The developer has put in a significant effort. This code might not work properly. Use at your own risk.

This code has not been reviewed by professional coders, it is a large task. If there are tests available in the codebase, please review those and their code.

---

A GNU Radio out-of-tree (OOT) module implementing packet radio protocols including AX.25, FX.25, and IL2P.

This is offering complete packet radio protocol support for GNU Radio applications.

## Features

### AX.25 Protocol
- Complete AX.25 v2.2 Link Layer implementation
- Support for both modulo 8 (standard) and modulo 128 (extended) sequence numbers
- Selective Reject (SREJ) for improved error recovery
- Extended frame sizes up to 2048 bytes (v2.2 feature)
- I, S, and U frame types with full state machine
- Frame Reject (FRMR) handling for protocol error recovery
- KISS TNC interface for hardware integration
- APRS support for position reporting and messaging
- Full address handling with callsigns and SSIDs
- Compatible with standard AX.25 implementations

### FX.25 Support
- Forward Error Correction (FEC) for AX.25 frames
- Full Reed-Solomon (255,k) codec implementation with error correction
- Multiple FEC types: RS(255,239), RS(255,223), RS(255,191), RS(255,159), RS(255,127), RS(255,95), RS(255,63), RS(255,31)
- Interleaving support for burst error correction
- Maintains backward compatibility with standard AX.25


### IL2P Protocol
- Improved Layer 2 Protocol implementation
- Full Reed-Solomon (255,k) codec with error correction
- Proper IL2P sync word (0xF15E48) and preamble (0x55)
- Data scrambling for improved bit transitions and clock recovery
- FEC types: RS(255,223), RS(255,239), RS(255,247)
- Enhanced reliability over noisy channels
- Modern replacement for AX.25 (not backward compatible)

### Adaptive Features
- **Link Quality Monitoring**: Real-time SNR, BER, and frame error rate monitoring
- **Automatic Rate Adaptation**: Dynamically adjusts modulation mode based on channel quality
- **Modulation Negotiation**: Full protocol implementation for negotiating modulation modes between stations
- **Inter-Station Communication**: KISS protocol extensions (0x10-0x14) for negotiation and quality feedback
- **Automatic Negotiation Triggers**: Automatically negotiates mode changes when adaptive rate control switches modes
- **Quality Feedback**: Bidirectional quality metric exchange between stations
- **Multi-Mode Support**: Supports four tiers of modulation modes:
  - Tier 1 (1,200 baud): 2FSK, 4FSK, 8FSK, 16FSK
  - Tier 2 (12,500 baud PSK): BPSK, QPSK, 8PSK
  - Tier 3 (12,500 baud QAM): 16-QAM, 64-QAM, 256-QAM
  - Tier 4 (Broadband SOQPSK): 1-40 Mbps modes for 23cm/13cm bands
- **Quality-Based Switching**: Intelligent mode selection based on SNR, BER, and link quality scores


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

#### Optional Dependencies

For PlutoSDR PTT control functionality:
```bash
pip install pylibiio
```

On some systems (e.g., newer Debian/Ubuntu), you may need to use:
```bash
pip install --break-system-packages pylibiio
```

This package provides Python bindings for libiio, required to control GPIO pins on PlutoSDR devices. See `docs/PTT_CONTROL.md` for more information.

## Usage

### GNU Radio Companion

The module provides blocks in the "Packet Protocols" category:

**Core Protocol Blocks:**
- **AX.25 Encoder**: Encodes data into AX.25 frames
- **AX.25 Decoder**: Decodes AX.25 frames
- **KISS TNC**: Interface to KISS-compatible TNCs with PTT control
- **FX.25 Encoder**: Encodes with forward error correction
- **FX.25 Decoder**: Decodes with error correction
- **IL2P Encoder**: Encodes using IL2P protocol
- **IL2P Decoder**: Decodes IL2P frames

**Adaptive Features Blocks:**
- **Link Quality Monitor**: Monitors SNR, BER, and frame error rate
- **Adaptive Rate Control**: Automatically adjusts modulation mode based on link quality
- **Modulation Negotiation**: Handles modulation mode negotiation between stations
- **Adaptive Modulator**: Python hierarchical block that switches between modulation modes automatically
- **Modulation Switch**: Switches between multiple modulation inputs (uses GNU Radio's selector block in GRC)

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

- `ax25_kiss_example.grc`: AX.25 with KISS TNC interface and PTT control
- `fx25_fec_example.grc`: FX.25 forward error correction
- `il2p_example.grc`: IL2P protocol demonstration

### PTT Control

The KISS TNC block includes built-in PTT (Push To Talk) control for packet radio transmitters. PTT control can be configured to use either the DTR or RTS line of the serial port.

**Using PTT Control in GNU Radio Companion:**

1. Add a KISS TNC block to your flowgraph
2. Configure the block parameters (device, baud rate, etc.)
3. After generating the flowgraph, modify the generated Python code to enable PTT:
   ```python
   self.packet_protocols_kiss_tnc_0.set_ptt_enabled(True)
   self.packet_protocols_kiss_tnc_0.set_ptt_use_dtr(False)  # Use RTS (default)
   ```
4. The block will automatically key the transmitter before sending data and unkey after transmission

**Using PTT Control in Python:**

```python
import gnuradio.packet_protocols as pp

# Create KISS TNC with PTT control
tnc = pp.kiss_tnc(
    device="/dev/ttyUSB0",
    baud_rate=9600
)

# Enable PTT control
tnc.set_ptt_enabled(True)

# Use DTR for PTT (optional, default is RTS)
tnc.set_ptt_use_dtr(False)  # Use RTS (default)

# Manually control PTT if needed
tnc.set_ptt(True)   # Key transmitter
tnc.set_ptt(False)  # Unkey transmitter

# Check PTT state
is_keyed = tnc.get_ptt()
```

**PTT Timing:**

The KISS TNC block respects the configured TX delay and TX tail parameters:
- **TX Delay**: Time to wait after keying PTT before transmitting (in 10ms units)
- **TX Tail**: Time to wait after transmission before unkeying PTT (in 10ms units)

These parameters can be set using `set_tx_delay()` and `set_tx_tail()` methods.

### Alternative PTT Control Methods

In GNU Radio packet radio applications, PTT control can be implemented using several methods depending on your hardware setup:

#### 1. Hardware-Specific Blocks (SDR Transmitters)

For Software Defined Radio (SDR) transmitters, PTT is typically controlled through hardware-specific blocks:

**gr-osmosdr (Osmocom SDR):**

```python
from gnuradio import osmosdr

# Create osmocom source/sink
sdr = osmosdr.sink(args="numchan=1")

# PTT control is typically handled automatically by the driver
# Some devices support GPIO pins for PTT control
# Check your device documentation for specific GPIO pin numbers

# Example for HackRF with GPIO PTT
sdr.set_gpio(0x01)  # Set GPIO pin 0 for PTT (device-specific)
```

**gr-uhd (Ettus USRP):**

```python
from gnuradio import uhd

# Create USRP sink
usrp_sink = uhd.usrp_sink(
    device_args="",
    stream_args=uhd.stream_args('fc32', 'sc16')
)

# USRP devices typically control PTT via front panel or API
# For GPIO-based PTT control:
usrp_sink.set_gpio_attr("RXA", "CTRL", 0x01, 0x01)  # Set GPIO for PTT
```

**Integration with Packet Protocols:**

When using SDR hardware, connect your packet encoder output directly to the SDR sink. PTT control should be coordinated with the data stream using tagged streams or message passing:

```python
from gnuradio import blocks, packet_protocols, osmosdr

# Create packet encoder
encoder = packet_protocols.ax25_encoder(...)

# Create SDR sink
sdr = osmosdr.sink(args="...")

# Use tagged stream to coordinate PTT with packet transmission
# The SDR driver handles PTT automatically based on data presence
blocks.connect(encoder, sdr)
```

#### 2. Custom GPIO Control Blocks for Hardware TNCs

For hardware TNCs that require GPIO-based PTT control (e.g., Raspberry Pi GPIO, BeagleBone, or custom hardware), you can create custom blocks or use existing GPIO libraries:

**Using Python Blocks for GPIO Control:**

```python
from gnuradio import gr, blocks
import RPi.GPIO as GPIO  # For Raspberry Pi

class gpio_ptt_block(gr.sync_block):
    def __init__(self, gpio_pin=18):
        gr.sync_block.__init__(
            self,
            name="GPIO PTT Control",
            in_sig=[gr.sizeof_char],
            out_sig=[gr.sizeof_char]
        )
        self.gpio_pin = gpio_pin
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(gpio_pin, GPIO.OUT)
        GPIO.output(gpio_pin, GPIO.LOW)  # Start unkeyed
        self.ptt_state = False
        
    def work(self, input_items, output_items):
        # Key PTT when data is present
        if len(input_items[0]) > 0 and not self.ptt_state:
            GPIO.output(self.gpio_pin, GPIO.HIGH)
            self.ptt_state = True
            
        # Copy input to output
        output_items[0][:] = input_items[0][:]
        
        # Unkey PTT after delay (implement timing logic)
        # This is simplified - real implementation needs proper timing
        
        return len(output_items[0])
        
    def stop(self):
        GPIO.output(self.gpio_pin, GPIO.LOW)
        GPIO.cleanup()
        return True
```

**Using Message-Based PTT Control:**

For more precise timing control, use message passing to coordinate PTT with packet transmission:

```python
from gnuradio import gr, blocks
import time

class message_ptt_block(gr.basic_block):
    def __init__(self, gpio_pin=18):
        gr.basic_block.__init__(
            self,
            name="Message PTT Control",
            in_sig=None,
            out_sig=None
        )
        self.gpio_pin = gpio_pin
        self.message_port_register_in(pmt.intern("packet"))
        self.set_msg_handler(pmt.intern("packet"), self.handle_packet)
        # Initialize GPIO...
        
    def handle_packet(self, msg):
        # Key PTT
        GPIO.output(self.gpio_pin, GPIO.HIGH)
        time.sleep(0.01)  # TX delay
        
        # Send packet notification to encoder
        # (implementation depends on your flowgraph)
        
        # Unkey PTT after transmission
        time.sleep(packet_duration)
        GPIO.output(self.gpio_pin, GPIO.LOW)
```

**Using Linux GPIO (sysfs) for Generic Hardware:**

For systems with Linux GPIO support (sysfs or libgpiod):

```python
import os

class sysfs_gpio_ptt:
    def __init__(self, gpio_number):
        self.gpio_number = gpio_number
        self.gpio_path = f"/sys/class/gpio/gpio{gpio_number}"
        
        # Export GPIO if not already exported
        if not os.path.exists(self.gpio_path):
            with open("/sys/class/gpio/export", "w") as f:
                f.write(str(gpio_number))
        
        # Set direction to output
        with open(f"{self.gpio_path}/direction", "w") as f:
            f.write("out")
            
    def set_ptt(self, state):
        with open(f"{self.gpio_path}/value", "w") as f:
            f.write("1" if state else "0")
```

**Integration Example:**

```python
from gnuradio import gr, blocks, packet_protocols

class top_block(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        
        # Create packet encoder
        encoder = packet_protocols.ax25_encoder(...)
        
        # Create GPIO PTT control block
        gpio_ptt = gpio_ptt_block(gpio_pin=18)
        
        # Create data source
        source = blocks.vector_source_b([...])
        
        # Connect: source -> encoder -> gpio_ptt -> sink
        self.connect(source, encoder, gpio_ptt, blocks.null_sink(gr.sizeof_char))
```

#### 3. Choosing the Right PTT Control Method

- **Built-in KISS TNC PTT**: Use when connecting to hardware TNCs via serial port (DTR/RTS lines). Simplest option for traditional TNC hardware.

- **SDR Hardware Blocks**: Use when transmitting directly with SDR hardware (HackRF, USRP, RTL-SDR with upconverter). PTT is typically handled by the driver or via GPIO pins.

- **Custom GPIO Blocks**: Use when you need direct GPIO control for custom hardware, Raspberry Pi projects, or when the built-in serial port PTT doesn't meet your needs.

- **Message-Based Control**: Use when you need precise timing control and want to coordinate PTT with specific packet events rather than continuous data streams.

For detailed documentation on all PTT control methods, including complete code examples and hardware-specific guidance, see [docs/PTT_CONTROL.md](docs/PTT_CONTROL.md).

## Adaptive Features

The module includes advanced adaptive features for optimizing link performance:

### Link Quality Monitoring

Monitor real-time link quality metrics:

```python
from gnuradio import packet_protocols

# Create link quality monitor
quality_monitor = packet_protocols.link_quality_monitor(
    alpha=0.1,        # Smoothing factor
    update_period=1000  # Update every 1000 samples
)

# Update metrics from external sources (e.g., GNU Radio SNR estimator)
quality_monitor.update_snr(15.5)  # SNR in dB
quality_monitor.update_ber(0.001)  # Bit error rate

# Get current metrics
snr = quality_monitor.get_snr()
ber = quality_monitor.get_ber()
quality_score = quality_monitor.get_quality_score()  # 0.0-1.0
```

### Automatic Rate Adaptation

Automatically adjust modulation mode based on link quality:

```python
from gnuradio import packet_protocols

# Create adaptive rate controller
rate_control = packet_protocols.adaptive_rate_control(
    initial_mode=packet_protocols.modulation_mode_t.MODE_2FSK,  # Default: Bell 202 for maximum compatibility
    enable_adaptation=True,
    hysteresis_db=2.0,  # Prevent rapid switching
    enable_tier4=False  # Tier 4 disabled by default (only enable for 23cm/13cm broadband)
)

# Update quality (typically from link quality monitor)
rate_control.update_quality(
    snr_db=15.0,
    ber=0.001,
    quality_score=0.8
)

# Get current mode and data rate
current_mode = rate_control.get_modulation_mode()
data_rate = rate_control.get_data_rate()  # bits per second
```

### Modulation Negotiation

Negotiate modulation modes between stations:

```python
from gnuradio import packet_protocols

# Create modulation negotiator
negotiator = packet_protocols.modulation_negotiation(
    station_id="N0CALL",
    supported_modes=[
        packet_protocols.modulation_mode_t.MODE_2FSK,
        packet_protocols.modulation_mode_t.MODE_4FSK,
        packet_protocols.modulation_mode_t.MODE_8FSK
    ],
    negotiation_timeout_ms=5000
)

# Initiate negotiation with remote station
negotiator.initiate_negotiation(
    remote_station_id="N1CALL",
    proposed_mode=packet_protocols.modulation_mode_t.MODE_2FSK  # Start with Bell 202 for compatibility
)

# Check negotiated mode
if not negotiator.is_negotiating():
    mode = negotiator.get_negotiated_mode()
```

### Supported Modulation Modes

The system supports four tiers of modulation modes, organized by baud rate and envelope characteristics:

#### Tier 1 - Robust (1,200 baud, constant envelope):
- **2FSK**: Binary FSK (1,200 bps, most robust) - Bell 202 / AX.25 standard
- **4FSK**: 4-level FSK (2,400 bps)
- **8FSK**: 8-level FSK (3,600 bps)
- **16FSK**: 16-level FSK (4,800 bps)

#### Tier 2 - Medium (12,500 baud, constant envelope):
- **BPSK @ 12.5k**: Binary PSK (12,500 bps)
- **QPSK @ 12.5k**: Quadrature PSK (25,000 bps)
- **8PSK @ 12.5k**: 8-PSK (37,500 bps)

#### Tier 3 - High (12,500 baud, variable envelope):
- **16-QAM @ 12.5k**: 16-QAM (50,000 bps / 50 kbps)
- **64-QAM @ 12.5k**: 64-QAM (75,000 bps / 75 kbps)
- **256-QAM @ 12.5k**: 256-QAM (100,000 bps / 100 kbps)

#### Tier 4 - Broadband (23cm/13cm bands, SOQPSK constant envelope) - **DISABLED BY DEFAULT**:
- **SOQPSK 1M**: SOQPSK @ 781 kbaud (1 Mbps) - ~1 MHz bandwidth
- **SOQPSK 5M**: SOQPSK @ 3.9 Mbaud (5 Mbps) - ~5 MHz bandwidth
- **SOQPSK 10M**: SOQPSK @ 7.8 Mbaud (10 Mbps) - ~10 MHz bandwidth
- **SOQPSK 20M**: SOQPSK @ 15.6 Mbaud (20 Mbps) - ~20 MHz bandwidth
- **SOQPSK 40M**: SOQPSK @ 31.3 Mbaud (40 Mbps) - ~40 MHz bandwidth

**IMPORTANT: Tier 4 modes are DISABLED BY DEFAULT to prevent accidental out-of-bandwidth transmissions on standard 12.5 kHz VHF/UHF channels. These modes require 1-40 MHz bandwidth and are ONLY suitable for 23cm (1.2 GHz) and 13cm (2.4 GHz) amateur radio bands with SDR hardware. Enabling Tier 4 on standard narrowband channels will cause interference and violate regulations.**

#### Legacy Modes (lower baud rates):
- **BPSK**: Binary PSK (1,200 bps)
- **QPSK**: Quadrature PSK (2,400 bps)
- **8PSK**: 8-PSK (3,600 bps)
- **16-QAM**: 16-QAM (9,600 bps @ 2,400 baud)
- **64-QAM @ 6.25k**: 64-QAM (37,500 bps @ 6,250 baud)

**Default Mode**: 2FSK (Bell 202 / AX.25) at 1,200 bps - most common for packet radio

#### Tier 4 Bandwidth Requirements

**WARNING: Tier 4 modes are DISABLED BY DEFAULT. They require explicit enablement via `enable_tier4=True` parameter.**

**Be mindful of the bandwidth needed for Tier 4 modes:**

| Rate | Symbol Rate | Bandwidth Needed | Band |
|------|-------------|------------------|------|
| 1 Mbps | 781 kbaud | ~1 MHz | 23cm/13cm |
| 5 Mbps | 3.9 Mbaud | ~5 MHz | 23cm/13cm |
| 10 Mbps | 7.8 Mbaud | ~10 MHz | 23cm/13cm |
| 20 Mbps | 15.6 Mbaud | ~20 MHz | 13cm preferred |
| 40 Mbps | 31.3 Mbaud | ~40 MHz | 13cm only |

**Why Tier 4 is Disabled by Default:**
- Tier 4 modes require 1-40 MHz bandwidth, far exceeding standard 12.5 kHz VHF/UHF channel spacing
- Using Tier 4 on standard narrowband channels will cause severe out-of-bandwidth interference
- These modes are ONLY suitable for broadband segments in 23cm (1.2 GHz) and 13cm (2.4 GHz) bands
- Requires Software Defined Radio (SDR) hardware capable of wide bandwidth operation
- Must be explicitly enabled to prevent accidental misuse

**To Enable Tier 4 (Use with Extreme Caution):**

```python
from gnuradio import packet_protocols

# Only enable Tier 4 if operating on 23cm/13cm broadband segments
rate_control = packet_protocols.adaptive_rate_control(
    initial_mode=packet_protocols.modulation_mode_t.MODE_2FSK,
    enable_adaptation=True,
    hysteresis_db=2.0,
    enable_tier4=True  # Only enable for broadband 23cm/13cm operation
)

# Or enable/disable dynamically
rate_control.set_tier4_enabled(True)  # Use with caution
```

Tier 4 modes use SOQPSK (Shaped Offset Quadrature Phase Shift Keying) which provides constant envelope characteristics suitable for non-linear power amplifiers.

#### Why Bell 202 (2FSK) is the Default

The system defaults to **2FSK (Bell 202 / AX.25) at 1,200 bps** for several critical reasons:

1. **Maximum Compatibility**: Bell 202 is the de facto standard for packet radio. Virtually all packet radio hardware, software, and networks support this mode, ensuring your station can communicate with the widest range of equipment.

2. **Best Link Reliability**: 2FSK is the most robust modulation scheme, providing reliable communication even in poor propagation conditions, high noise environments, and during atmospheric disturbances. This ensures initial link establishment and maintains connectivity when channel conditions degrade.

3. **Industry Standard**: Bell 202 is the most common packet radio mode worldwide. Using it as the default ensures your station follows established conventions and can participate in existing packet radio networks without configuration changes.

4. **Graceful Degradation**: Starting at the most robust mode allows the adaptive system to establish a link first, then negotiate and adapt to higher speeds as channel conditions permit. This prevents connection failures that might occur if starting at higher, less robust modes.

5. **Interoperability**: Many packet radio systems, digipeaters, and BBSs expect Bell 202 as the initial mode. Starting with 2FSK ensures compatibility with legacy systems and maximizes the chances of successful communication.

The adaptive system will automatically upgrade to higher speeds (up to 100 kbps with 256-QAM) when channel conditions allow, but always begins with the most reliable mode to ensure successful link establishment.

### Integration with GNU Radio Blocks

The adaptive features integrate with existing GNU Radio modulation blocks. The system includes:

- **`adaptive_modulator` block**: Python hierarchical block that automatically switches between modulation modes
- **`modulation_switch` block**: Helper block for programmatic use with adaptive_rate_control (uses GNU Radio's selector in GRC)
- **Complete integration examples**: Working flowgraphs with all components connected
- **Step-by-step guides**: Detailed documentation for hooking everything up
- **GNU Radio Companion blocks**: All blocks are available in GRC with full parameter configuration

**Documentation:**
- **[GRC Blocks Guide](docs/GRC_BLOCKS.md)** - Complete list of all GNU Radio Companion blocks
- **[Quick Start Guide](docs/QUICK_START_ADAPTIVE.md)** - Get started in 5 minutes
- **[Complete Integration Guide](docs/COMPLETE_INTEGRATION_GUIDE.md)** - Step-by-step hookup instructions
- **[Adaptive Features Guide](docs/ADAPTIVE_FEATURES.md)** - Feature overview and GNU Radio block integration
- **[Modulation Control Guide](docs/MODULATION_CONTROL.md)** - Mode control details
- **[PTT Control Guide](docs/PTT_CONTROL.md)** - PTT control documentation

**Examples:**
- **[Adaptive Modulation Example](examples/adaptive_modulation_example.py)** - Complete working example with all components

The system integrates with:
- GNU Radio's `probe_mpsk_snr_est_c` for SNR estimation
- GNU Radio's FSK, PSK, and QAM modulation blocks
- Custom `modulation_switch` block for automatic mode switching

## Protocol Details

### AX.25
- AX.25 v2.2 Link Layer protocol implementation
- Supports both modulo 8 (standard) and modulo 128 (extended) sequence numbers
- Selective Reject (SREJ) for efficient error recovery
- Extended frame sizes up to 2048 bytes
- Supports both connected and unconnected modes
- Full state machine with Frame Reject (FRMR) handling
- KISS interface for hardware TNCs
- Full address and control field handling
- Built-in PTT (Push To Talk) control via serial port DTR/RTS lines
- Compatible with Dire Wolf and standard AX.25 implementations

### FX.25
- Extends AX.25 with forward error correction
- Full Reed-Solomon (255,k) codec implementation (not stubs)
- Multiple Reed-Solomon code options matching FX.25 specification
- Interleaving for burst error protection
- Error correction capability up to (n-k)/2 errors per codeword
- Backward compatible with standard AX.25
- Compatible with Dire Wolf FX.25 implementation

### IL2P
- Modern packet radio protocol (not backward compatible with AX.25)
- Full Reed-Solomon (255,k) codec with error correction
- Proper IL2P frame structure with sync word and preamble
- Data scrambling for improved transmission characteristics
- Enhanced error correction capabilities
- Improved performance over noisy channels
- Designed as modern AX.25 replacement

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
- Inspired by Dire Wolf and compatible with AX.25, FX.25, and IL2P implementations
- GNU Radio community for excellent framework
- Reed-Solomon implementation based on standard algorithms (Berlekamp-Massey, Chien search, Forney)

## Security Testing

This module has undergone comprehensive security testing using dictionary-guided fuzzing and Scapy-based attack simulations:

- **Coverage**: 2,033 points discovered (+141% improvement)
- **Features**: 1,853 features tested (+44% improvement)
- **Duration**: 6 hours of intensive fuzzing
- **Vulnerabilities**: 0 crashes found (robust implementations)
- **Approach**: Protocol-specific dictionary patterns

For detailed results, see [Fuzzing Results Report](fuzzing-results.md).  
Scapy attack vector tests and parser stress tests live in `security/scapy_tests/` (see [security/scapy_tests/README.md](security/scapy_tests/README.md)) and can be executed via:

```bash
cd security/scapy_tests
./run_scapy_tests.sh
```

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

### Version 1.2.0
- **AX.25 v2.2 Link Layer Support**:
  - Added modulo 128 (extended) sequence number support
  - Implemented Selective Reject (SREJ) for improved error recovery
  - Extended frame sizes up to 2048 bytes (v2.2 feature)
  - Full Frame Reject (FRMR) handling
  - Enhanced state machine with extended mode support
  - Compatible with AX.25 v2.2 implementation
- **Full Reed-Solomon Implementation**:
  - Replaced stub implementations with complete RS(255,k) codec
  - Full Galois Field GF(256) arithmetic implementation
  - Berlekamp-Massey algorithm for error locator polynomial
  - Chien search for error position finding
  - Forney algorithm for error value calculation
  - Error correction capability up to (n-k)/2 errors per codeword
- **FX.25 Improvements**:
  - Updated to use correct RS(255,k) codes matching FX.25 specification
  - Support for all standard FX.25 FEC types
- **IL2P Improvements**:
  - Added proper IL2P sync word (0xF15E48) and preamble (0x55)
  - Implemented data scrambling for improved bit transitions
  - Full Reed-Solomon error correction
- **Testing**:
  - All unit tests passing (100% pass rate)
  - Comprehensive test coverage for all protocols
  - Fixed test issues and improved test reliability

### Version 1.1.0
- Added adaptive modulation features:
  - Link Quality Monitor block
  - Adaptive Rate Control block
  - Modulation Negotiation block with full KISS protocol extensions
  - Adaptive Modulator hierarchical block
  - Modulation Switch block
- Added PTT (Push To Talk) control to KISS TNC block
- Implemented KISS protocol extensions for negotiation (commands 0x10-0x14):
  - Negotiation request/response/acknowledgment
  - Mode change notifications
  - Quality feedback messages
- Inter-station communication for coordinated mode adaptation
- Automatic negotiation triggers when mode changes
- Frame encoding/decoding for all negotiation message types
- Message port integration between KISS TNC and Modulation Negotiation
- All blocks available in GNU Radio Companion
- Comprehensive documentation and examples
- Unit tests for all adaptive blocks

### Version 1.0.0
- Initial release
- AX.25 protocol implementation
- FX.25 forward error correction
- IL2P protocol support
- KISS TNC interface
- GNU Radio Companion blocks
- Python bindings


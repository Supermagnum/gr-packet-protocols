# GNU Radio Companion Blocks

This document lists all available blocks in GNU Radio Companion for gr-packet-protocols.

## Core Protocol Blocks

### AX.25 Encoder
- **ID**: `packet_protocols_ax25_encoder`
- **Category**: `[Packet Protocols]`
- **Input**: Byte stream
- **Output**: Byte stream (AX.25 encoded)
- **Parameters**:
  - Destination Callsign (string)
  - Destination SSID (string)
  - Source Callsign (string)
  - Source SSID (string)
  - Digipeaters (string, optional)
  - Command/Response (bool)
  - Poll/Final (bool)

### AX.25 Decoder
- **ID**: `packet_protocols_ax25_decoder`
- **Category**: `[Packet Protocols]`
- **Input**: Byte stream (AX.25 encoded)
- **Output**: Byte stream (decoded)
- **Parameters**: None

### KISS TNC
- **ID**: `packet_protocols_kiss_tnc`
- **Category**: `[Packet Protocols]`
- **Input**: Byte stream
- **Output**: Byte stream
- **Message Ports**:
  - `negotiation_out`: Forwards received negotiation frames (commands 0x10-0x14)
- **Parameters**:
  - Device (string, e.g., `/dev/ttyUSB0`)
  - Baud Rate (int, default: 9600)
  - Hardware Flow Control (bool)
  - Enable PTT Control (bool)
  - Use DTR for PTT (bool, shown when PTT enabled)
- **Features**:
  - Supports KISS protocol extensions for modulation negotiation
  - Automatically forwards negotiation frames to message port
  - PTT control via DTR or RTS line

### FX.25 Encoder
- **ID**: `packet_protocols_fx25_encoder`
- **Category**: `[Packet Protocols]`
- **Input**: Byte stream
- **Output**: Byte stream (FX.25 encoded with FEC)
- **Parameters**:
  - FEC Type (int, default: 2)
  - Interleaver Depth (int, default: 1)
  - Add Checksum (bool)

### FX.25 Decoder
- **ID**: `packet_protocols_fx25_decoder`
- **Category**: `[Packet Protocols]`
- **Input**: Byte stream (FX.25 encoded)
- **Output**: Byte stream (decoded)
- **Parameters**: None

### IL2P Encoder
- **ID**: `packet_protocols_il2p_encoder`
- **Category**: `[Packet Protocols]`
- **Input**: Byte stream
- **Output**: Byte stream (IL2P encoded)
- **Parameters**:
  - Destination Callsign (string)
  - Destination SSID (string)
  - Source Callsign (string)
  - Source SSID (string)
  - FEC Type (int, default: 1)
  - Add Checksum (bool)

### IL2P Decoder
- **ID**: `packet_protocols_il2p_decoder`
- **Category**: `[Packet Protocols]`
- **Input**: Byte stream (IL2P encoded)
- **Output**: Byte stream (decoded)
- **Parameters**: None

## Adaptive Features Blocks

### Link Quality Monitor
- **ID**: `packet_protocols_link_quality_monitor`
- **Category**: `[Packet Protocols]`
- **Input**: Byte stream (pass-through)
- **Output**: Byte stream (pass-through)
- **Parameters**:
  - Smoothing Factor (Alpha) (float, 0.0-1.0, default: 0.1)
  - Update Period (samples) (int, default: 1000)
- **Description**: Monitors SNR, BER, and frame error rate. Provides quality metrics for adaptive rate control.

### Adaptive Rate Control
- **ID**: `packet_protocols_adaptive_rate_control`
- **Category**: `[Packet Protocols]`
- **Input**: Byte stream (pass-through)
- **Output**: Byte stream (pass-through)
- **Parameters**:
  - Initial Modulation Mode (enum, default: MODE_4FSK)
    - Options: 2FSK, 4FSK, 8FSK, 16FSK, BPSK, QPSK, 8PSK, 16-QAM, 64-QAM
  - Enable Adaptation (bool, default: True)
  - Hysteresis (dB) (float, default: 2.0)
- **Description**: Automatically adjusts modulation mode based on link quality metrics.

### Modulation Negotiation
- **ID**: `packet_protocols_modulation_negotiation`
- **Category**: `[Packet Protocols]`
- **Input**: Byte stream (pass-through)
- **Output**: Byte stream (pass-through)
- **Message Ports**:
  - `negotiation_in`: Receives negotiation frames from KISS TNC
- **Parameters**:
  - Station ID (Callsign) (string, default: N0CALL)
  - Supported Modes (comma-separated string, default: MODE_2FSK,MODE_4FSK,MODE_8FSK)
  - Negotiation Timeout (ms) (int, default: 5000)
- **Description**: Handles negotiation of modulation modes between stations using KISS protocol extensions (commands 0x10-0x14). Supports automatic negotiation triggers and quality feedback. Connect KISS TNC's `negotiation_out` message port to this block's `negotiation_in` port.
- **Features**:
  - Full KISS protocol extension support
  - Frame encoding/decoding for all negotiation message types
  - Automatic negotiation when adaptive rate control changes modes
  - Quality feedback transmission
  - Message port integration with KISS TNC

### Adaptive Modulator
- **ID**: `packet_protocols_adaptive_modulator`
- **Category**: `[Packet Protocols]`
- **Input**: Byte stream
- **Output**: Complex stream
- **Parameters**:
  - Initial Modulation Mode (enum, default: MODE_4FSK)
  - Samples per Symbol (int, default: 2)
  - Enable Adaptation (bool, default: True)
  - Hysteresis (dB) (float, default: 2.0)
- **Description**: Python hierarchical block that automatically switches between modulation modes. This is the recommended block for GRC flowgraphs.

### Modulation Switch
- **ID**: `packet_protocols_modulation_switch`
- **Category**: `[Packet Protocols]`
- **Input**: Complex stream (multiple inputs via multiplicity)
- **Output**: Complex stream
- **Parameters**:
  - Number of Inputs (int, 2-16, default: 4)
  - Selected Input Index (int, 0-15, default: 0)
- **Description**: Switches between multiple modulation inputs. Uses GNU Radio's selector block internally. For programmatic use with adaptive_rate_control, use the Python `modulation_switch` class directly.

## Using Blocks in GRC

1. **Open GNU Radio Companion**
2. **Find blocks** in the block tree under `[Packet Protocols]`
3. **Drag blocks** into the flowgraph canvas
4. **Configure parameters** by double-clicking blocks
5. **Connect blocks** by clicking and dragging between ports

## Block Availability

All blocks are available after installation:
```bash
cd build
cmake ..
make
sudo make install
```

Restart GNU Radio Companion after installation to see the new blocks.

## Examples

Example flowgraphs using these blocks:
- `examples/ax25_kiss_example.grc` - AX.25 with KISS TNC and PTT control
- `examples/fx25_fec_example.grc` - FX.25 forward error correction
- `examples/il2p_example.grc` - IL2P protocol demonstration
- `examples/adaptive_modulation_example.py` - Adaptive modulation system (Python)

## See Also

- [Complete Integration Guide](COMPLETE_INTEGRATION_GUIDE.md) - How to hook up adaptive blocks
- [Adaptive Features Guide](ADAPTIVE_FEATURES.md) - Feature overview
- [Modulation Control Guide](MODULATION_CONTROL.md) - Mode control details
- [PTT Control Guide](PTT_CONTROL.md) - PTT control documentation


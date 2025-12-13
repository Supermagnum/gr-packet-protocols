# Full-Duplex Adaptive Modulation System

This example demonstrates a complete full-duplex adaptive modulation system with bidirectional quality feedback and modulation negotiation.

## Files

- `adaptive_full_duplex_example.grc` - GNU Radio Companion flowgraph
- `adaptive_full_duplex_example.py` - Complete Python implementation with all connections

## Features

### 1. Link Quality Monitor Block
- **TX Path**: Monitors quality of transmitted data
- **RX Path**: Monitors quality of received data (SNR, BER, FER)
- Tracks SNR, BER, and frame error rate with exponential smoothing

### 2. Adaptive Rate Control Block
- Automatically selects optimal modulation mode based on link quality
- Uses hysteresis to prevent rapid mode switching
- Supports 9 modulation modes (2FSK through 64-QAM)

### 3. Modulation Negotiation Block
- Coordinates mode changes between stations
- Full KISS protocol extensions (commands 0x10-0x14):
  - `KISS_CMD_NEG_REQ (0x10)`: Negotiation request
  - `KISS_CMD_NEG_RESP (0x11)`: Negotiation response
  - `KISS_CMD_NEG_ACK (0x12)`: Negotiation acknowledgment
  - `KISS_CMD_MODE_CHANGE (0x13)`: Mode change notification
  - `KISS_CMD_QUALITY_FB (0x14)`: Quality feedback

### 4. Quality Feedback: Bidirectional Exchange
- Stations periodically exchange quality metrics (SNR, BER, quality score)
- Enables coordinated adaptation based on both local and remote conditions
- Automatic transmission every 5 seconds (configurable)

## Architecture

```
TX Path:
[Vector Source] -> [Throttle] -> [AX.25 Encoder] -> 
[Link Quality Monitor] -> [Adaptive Modulator] -> [File Sink/USRP]

RX Path:
[File Source/USRP] -> [Demodulator] -> [AX.25 Decoder] -> 
[Link Quality Monitor] -> [Null Sink]
         |
         v
[SNR Estimator] -> [Quality Updates]

Control:
[Adaptive Rate Control] <-> [Modulation Negotiation] <-> [KISS TNC]
         |                            |
         v                            v
[Quality Updates]          [Negotiation Frames]
```

## Message Port Connections

The GRC file structure is provided, but **message port connections must be added in the generated Python code**:

### 1. KISS TNC to Modulation Negotiation

```python
# Connect KISS TNC negotiation_out to Modulation Negotiation negotiation_in
self.msg_connect(
    self.kiss_tnc, "negotiation_out",
    self.negotiator, "negotiation_in"
)
```

### 2. SNR Estimator to Quality Monitor

```python
# Connect SNR estimator to quality update handler
self.rx_snr_estimator.message_port_register_out(pmt.intern("snr"))
self.msg_connect(
    self.rx_snr_estimator, "snr",
    self, pmt.intern("snr_update")
)

# Register message handler
self.message_port_register_in(pmt.intern("snr_update"))
self.set_msg_handler(pmt.intern("snr_update"), self.handle_snr_update)
```

### 3. Quality Update Handler

```python
def handle_snr_update(self, msg):
    """Handle SNR update from estimator"""
    snr_db = pmt.to_double(msg)
    self.rx_quality_monitor.update_snr(snr_db)
    self.tx_quality_monitor.update_snr(snr_db)
```

## Required Python Code Additions

When using the GRC file, you must add the following to the generated Python code:

### 1. Quality Update Thread

```python
import threading
import time

def update_quality():
    while True:
        time.sleep(1.0)
        snr_db = self.rx_quality_monitor.get_snr()
        ber = self.rx_quality_monitor.get_ber()
        quality_score = self.rx_quality_monitor.get_quality_score()
        
        self.rate_control.update_quality(snr_db, ber, quality_score)
        
        current_mode = self.rate_control.get_modulation_mode()
        self.tx_adaptive_mod.set_modulation_mode(current_mode)

threading.Thread(target=update_quality, daemon=True).start()
```

### 2. Quality Feedback Thread

```python
def send_quality_feedback():
    while True:
        time.sleep(5.0)
        snr = self.rx_quality_monitor.get_snr()
        ber = self.rx_quality_monitor.get_ber()
        quality = self.rx_quality_monitor.get_quality_score()
        
        if snr > 0.0:
            self.negotiator.send_quality_feedback(
                "N1CALL",  # Remote station ID
                snr, ber, quality
            )

threading.Thread(target=send_quality_feedback, daemon=True).start()
```

### 3. Frame Success/Error Recording

```python
# Call when frame is successfully decoded
self.rx_quality_monitor.record_frame_success()

# Call when frame fails to decode
self.rx_quality_monitor.record_frame_error()
```

### 4. Enable Automatic Negotiation

```python
# Enable automatic negotiation when mode changes
self.negotiator.set_auto_negotiation_enabled(True, self.rate_control)
```

## KISS Protocol Extensions

The system uses KISS protocol extensions for negotiation:

### Negotiation Request (0x10)
- Station ID length (1 byte)
- Station ID (variable)
- Proposed mode (1 byte)
- Number of supported modes (1 byte)
- Supported modes list (1 byte each)

### Negotiation Response (0x11)
- Station ID length (1 byte)
- Station ID (variable)
- Accepted flag (1 byte: 0=rejected, 1=accepted)
- Negotiated mode (1 byte)

### Mode Change (0x13)
- Station ID length (1 byte)
- Station ID (variable)
- New mode (1 byte)

### Quality Feedback (0x14)
- Station ID length (1 byte)
- Station ID (variable)
- SNR in dB (4 bytes, IEEE 754 float)
- BER (4 bytes, IEEE 754 float)
- Quality score (4 bytes, IEEE 754 float)

## Usage

### Option 1: Use Complete Python Implementation

```bash
python3 adaptive_full_duplex_example.py
```

This includes all message port connections and quality update mechanisms.

### Option 2: Use GRC File

1. Open `adaptive_full_duplex_example.grc` in GNU Radio Companion
2. Generate the flowgraph
3. Modify the generated Python code to add:
   - Message port connections (see above)
   - Quality update thread
   - Quality feedback thread
   - Frame success/error recording

## Configuration

### Station IDs

Edit in the Python code:
```python
self.station_id = "N0CALL"
self.remote_station_id = "N1CALL"
```

### Supported Modes

Edit in GRC or Python:
```python
supported_modes=[
    packet_protocols.modulation_mode_t.MODE_2FSK,
    packet_protocols.modulation_mode_t.MODE_4FSK,
    packet_protocols.modulation_mode_t.MODE_8FSK,
    packet_protocols.modulation_mode_t.MODE_QPSK,
]
```

### Quality Update Period

In Link Quality Monitor:
- `alpha`: Smoothing factor (0.0-1.0, higher = faster response)
- `update_period`: Update every N samples

### Adaptation Parameters

In Adaptive Rate Control:
- `hysteresis_db`: Prevents rapid switching (default: 2.0 dB)
- `enable_adaptation`: Enable/disable automatic mode switching

## Hardware Integration

### For SDR (Software-Defined Radio)

Replace file sinks/sources with USRP blocks:
- TX: Replace `blocks_file_sink` with `uhd_usrp_sink`
- RX: Replace `blocks_file_source` with `uhd_usrp_source`

### For Hardware TNC

The KISS TNC block is included for negotiation frames, but note:
- KISS TNC expects **bytes** (packet data)
- Adaptive Modulator outputs **complex** samples (modulated signal)
- These are separate paths: use KISS TNC for negotiation, not for data path

## Troubleshooting

### Mode Not Switching
- Check that `enable_adaptation=True` in adaptive_rate_control
- Verify quality metrics are being updated regularly
- Check that SNR/BER measurements are valid (> 0)

### Negotiation Not Working
- Verify message port connections are set up
- Check that KISS TNC device is accessible
- Ensure both stations have compatible supported modes

### Quality Feedback Not Sending
- Check that SNR measurements are valid (snr > 0.0)
- Verify remote station ID is correct
- Check for errors in quality feedback thread

## See Also

- [Adaptive Features Guide](../docs/ADAPTIVE_FEATURES.md) - Detailed feature documentation
- [Adaptive System Architecture](../docs/ADAPTIVE_SYSTEM_ARCHITECTURE.md) - Architecture details
- [Complete Integration Guide](../docs/COMPLETE_INTEGRATION_GUIDE.md) - Integration instructions


# Half-Duplex Adaptive Modulation System

This example demonstrates a half-duplex adaptive modulation system with TX/RX switching, bidirectional quality feedback, and modulation negotiation.

## Files

- `adaptive_half_duplex_example.grc` - GNU Radio Companion flowgraph
- `adaptive_half_duplex_example.py` - Complete Python implementation with all connections

## Features

### 1. Link Quality Monitor Block
- **TX Path**: Monitors quality of transmitted data
- **RX Path**: Monitors quality of received data (SNR, BER, FER)
- Tracks SNR, BER, and frame error rate with exponential smoothing
- Quality updates only occur in RX mode

### 2. Adaptive Rate Control Block
- Automatically selects optimal modulation mode based on link quality
- Uses hysteresis to prevent rapid mode switching
- Supports 9 modulation modes (2FSK through 64-QAM)
- Mode selection based on RX quality measurements

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
- PTT is keyed briefly to send feedback frames

### 5. PTT Control (Push To Talk)
- Switches between TX and RX modes
- TX Mode: Routes adaptive modulator output to radio
- RX Mode: Routes radio input to demodulator
- Automatic PTT control based on data availability
- Manual PTT control via `set_ptt(True/False)` method

## Architecture

```
TX Mode (PTT Keyed):
[Vector Source] -> [Throttle] -> [AX.25 Encoder] -> 
[Link Quality Monitor] -> [Adaptive Modulator] -> 
[TX/RX Selector] -> [Radio Output]

RX Mode (PTT Unkeyed):
[Radio Input] -> [RX Input Selector] -> [Demodulator] -> 
[AX.25 Decoder] -> [Link Quality Monitor] -> [Null Sink]
         |
         v
[SNR Estimator] -> [Quality Updates]

Control:
[Adaptive Rate Control] <-> [Modulation Negotiation] <-> [KISS TNC]
         |                            |
         v                            v
[Quality Updates]          [Negotiation Frames]
         |
         v
[PTT Control Logic]
```

## Key Differences from Full-Duplex

1. **Single Radio Path**: One radio interface that switches between TX and RX
2. **PTT Control**: Push To Talk mechanism to switch modes
3. **Selector Blocks**: Route signals based on PTT state
4. **Quality Updates**: Only occur in RX mode
5. **Quality Feedback**: PTT is keyed briefly to send feedback

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

### 3. Quality Update Handler (RX Mode Only)

```python
def handle_snr_update(self, msg):
    """Handle SNR update from estimator"""
    snr_db = pmt.to_double(msg)
    # Only update if in RX mode
    with self.ptt_lock:
        if not self.ptt_state:  # RX mode
            self.rx_quality_monitor.update_snr(snr_db)
```

## Required Python Code Additions

When using the GRC file, you must add the following to the generated Python code:

### 1. PTT Control

```python
import threading

# PTT State (False = RX mode, True = TX mode)
self.ptt_state = False
self.ptt_lock = threading.Lock()

def set_ptt(self, state):
    """Set PTT state (True = TX mode, False = RX mode)"""
    with self.ptt_lock:
        if self.ptt_state != state:
            self.ptt_state = state
            
            # Update selectors
            if state:  # TX mode
                self.tx_rx_selector.set_input_index(0)  # Select TX path
                self.rx_input_selector.set_input_index(1)  # Disable RX
            else:  # RX mode
                self.tx_rx_selector.set_input_index(1)
                self.rx_input_selector.set_input_index(0)  # Enable RX
```

### 2. Quality Update Thread (RX Mode Only)

```python
def update_quality():
    while True:
        time.sleep(1.0)
        
        # Only update quality in RX mode
        with self.ptt_lock:
            if self.ptt_state:  # TX mode - skip
                continue
        
        snr_db = self.rx_quality_monitor.get_snr()
        ber = self.rx_quality_monitor.get_ber()
        quality_score = self.rx_quality_monitor.get_quality_score()
        
        self.rate_control.update_quality(snr_db, ber, quality_score)
        current_mode = self.rate_control.get_modulation_mode()
        self.tx_adaptive_mod.set_modulation_mode(current_mode)

threading.Thread(target=update_quality, daemon=True).start()
```

### 3. Quality Feedback Thread (with PTT)

```python
def send_quality_feedback():
    while True:
        time.sleep(5.0)
        
        # Only send feedback in RX mode
        with self.ptt_lock:
            if self.ptt_state:  # TX mode - skip
                continue
        
        snr = self.rx_quality_monitor.get_snr()
        ber = self.rx_quality_monitor.get_ber()
        quality = self.rx_quality_monitor.get_quality_score()
        
        if snr > 0.0:
            # Key PTT briefly to send feedback
            self.set_ptt(True)
            self.negotiator.send_quality_feedback(
                "N1CALL", snr, ber, quality
            )
            time.sleep(0.1)  # Brief transmission
            self.set_ptt(False)

threading.Thread(target=send_quality_feedback, daemon=True).start()
```

### 4. Frame Success/Error Recording (RX Mode Only)

```python
def record_frame_success(self):
    """Call this when a frame is successfully decoded"""
    with self.ptt_lock:
        if not self.ptt_state:  # Only in RX mode
            self.rx_quality_monitor.record_frame_success()

def record_frame_error(self):
    """Call this when a frame fails to decode"""
    with self.ptt_lock:
        if not self.ptt_state:  # Only in RX mode
            self.rx_quality_monitor.record_frame_error()
```

## PTT Control Logic

### Automatic PTT Control

The example includes a simple automatic PTT control that:
- Keys PTT every 10 seconds
- Transmits for 2 seconds
- Unkeys PTT to return to RX mode

In a real system, PTT control would be:
- Triggered by data availability
- Controlled by packet timing
- Coordinated with frame transmission

### Manual PTT Control

```python
# Key PTT (enter TX mode)
tb.set_ptt(True)

# Transmit data...

# Unkey PTT (enter RX mode)
tb.set_ptt(False)
```

## Selector Configuration

### TX/RX Mode Selector

- **Input 0**: TX path (adaptive modulator output)
- **Input 1**: RX path (from radio, typically null in TX mode)
- **Output**: Selected path to radio
- **Control**: Set via `set_input_index()` based on PTT state

### RX Input Selector

- **Input 0**: From radio (RX mode)
- **Input 1**: Null source (TX mode - no RX processing)
- **Output**: To demodulator
- **Control**: Set via `set_input_index()` based on PTT state

## Usage

### Option 1: Use Complete Python Implementation

```bash
python3 adaptive_half_duplex_example.py
```

This includes all message port connections, PTT control, and quality update mechanisms.

### Option 2: Use GRC File

1. Open `adaptive_half_duplex_example.grc` in GNU Radio Companion
2. Generate the flowgraph
3. Modify the generated Python code to add:
   - Message port connections
   - PTT control logic
   - Quality update thread (RX mode only)
   - Quality feedback thread (with PTT)
   - Frame success/error recording

## Configuration

### Station IDs

Edit in the Python code:
```python
self.station_id = "N0CALL"
self.remote_station_id = "N1CALL"
```

### PTT Control

Enable PTT in KISS TNC block:
- `ptt_enabled`: True
- `ptt_use_dtr`: False (use RTS) or True (use DTR)

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

## Hardware Integration

### For SDR (Software-Defined Radio)

Replace file sinks/sources with USRP blocks:
- Radio Output: Replace `blocks_file_sink` with `uhd_usrp_sink`
- Radio Input: Replace `blocks_file_source` with `uhd_usrp_source`

### PTT Control for SDR

For USRP, PTT control is typically handled by:
- GPIO pins on USRP
- External PTT interface
- Software control via USRP API

## Timing Considerations

### TX/RX Switching

- **TX Delay**: Delay before starting transmission (typically 10-50ms)
- **TX Tail**: Delay after transmission before switching to RX (typically 10-50ms)
- **RX Recovery**: Time for receiver to stabilize after TX (typically 10-20ms)

### Quality Feedback Timing

- Quality feedback is sent every 5 seconds
- PTT is keyed briefly (100ms) to send feedback
- Feedback should not interfere with normal data transmission

## Troubleshooting

### Mode Not Switching
- Check that `enable_adaptation=True` in adaptive_rate_control
- Verify quality metrics are being updated (only in RX mode)
- Check that SNR/BER measurements are valid (> 0)

### PTT Not Switching
- Verify selector blocks are properly configured
- Check that `set_input_index()` is being called
- Ensure PTT control thread is running

### Quality Updates Not Working
- Verify system is in RX mode when updates occur
- Check that SNR estimator is connected and working
- Ensure quality update thread is running

### Negotiation Not Working
- Verify message port connections are set up
- Check that KISS TNC device is accessible
- Ensure both stations have compatible supported modes
- Verify PTT is keyed when sending negotiation frames

## See Also

- [Full-Duplex Example](ADAPTIVE_FULL_DUPLEX_README.md) - Full-duplex version
- [Adaptive Features Guide](../docs/ADAPTIVE_FEATURES.md) - Detailed feature documentation
- [Adaptive System Architecture](../docs/ADAPTIVE_SYSTEM_ARCHITECTURE.md) - Architecture details
- [Complete Integration Guide](../docs/COMPLETE_INTEGRATION_GUIDE.md) - Integration instructions


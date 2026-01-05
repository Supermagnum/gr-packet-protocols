# Adaptive Features Implementation Guide

This document describes the adaptive features added to gr-packet-protocols, including link quality monitoring, automatic rate adaptation, and modulation negotiation.

## Overview

The adaptive features system consists of three main components:

1. **Link Quality Monitor**: Monitors SNR, BER, and frame error rate
2. **Adaptive Rate Control**: Automatically adjusts modulation mode based on link quality
3. **Modulation Negotiation**: Handles negotiation of modulation modes between stations

## Integration with GNU Radio Blocks

### Link Quality Monitoring

The link quality monitor integrates with existing GNU Radio blocks:

#### SNR Estimation

Use GNU Radio's `probe_mpsk_snr_est_c` block to estimate SNR:

```python
from gnuradio import digital

# Create SNR estimator
snr_estimator = digital.probe_mpsk_snr_est_c(
    type=digital.SNR_EST_SIMPLE,
    msg_nsamples=1000,
    alpha=0.001
)

# Connect to your signal path
# The estimator will emit messages with SNR values
```

#### BER Measurement

For BER measurement, use GNU Radio's BER measurement blocks or implement custom logic:

```python
from gnuradio import blocks

# Compare transmitted and received data streams
ber_measure = blocks.sub_ff()  # Subtract to find errors
```

#### Using the Link Quality Monitor

```python
from gnuradio import packet_protocols

# Create link quality monitor
quality_monitor = packet_protocols.link_quality_monitor(
    alpha=0.1,        # Smoothing factor
    update_period=1000  # Update every 1000 samples
)

# Update metrics from external sources
quality_monitor.update_snr(15.5)  # Update SNR in dB
quality_monitor.update_ber(0.001)  # Update BER

# Get current quality metrics
snr = quality_monitor.get_snr()
ber = quality_monitor.get_ber()
quality_score = quality_monitor.get_quality_score()
```

### Adaptive Rate Control

The adaptive rate control block automatically selects the best modulation mode:

```python
from gnuradio import packet_protocols

# Create adaptive rate controller
rate_control = packet_protocols.adaptive_rate_control(
    initial_mode=packet_protocols.modulation_mode_t.MODE_2FSK,  # Default: Bell 202 / AX.25
    enable_adaptation=True,
    hysteresis_db=2.0  # Prevent rapid switching
)

# Update quality metrics (typically from link quality monitor)
rate_control.update_quality(
    snr_db=15.0,
    ber=0.001,
    quality_score=0.8
)

# Get current mode and rate
current_mode = rate_control.get_modulation_mode()
data_rate = rate_control.get_data_rate()

# Get recommended mode for given conditions
recommended = rate_control.recommend_mode(snr_db=20.0, ber=0.0001)
```

### Modulation Modes

Supported modulation modes organized by tier:

#### Tier 1 - Robust (1,200 baud, constant envelope):
- `MODE_2FSK`: Binary FSK / Bell 202 / AX.25 (1,200 bps, most robust) - **Default**
- `MODE_4FSK`: 4-level FSK (2,400 bps)
- `MODE_8FSK`: 8-level FSK (3,600 bps)
- `MODE_16FSK`: 16-level FSK (4,800 bps)

#### Tier 2 - Medium (12,500 baud, constant envelope):
- `MODE_BPSK_12500`: Binary PSK @ 12.5k (12,500 bps)
- `MODE_QPSK_12500`: Quadrature PSK @ 12.5k (25,000 bps)
- `MODE_8PSK_12500`: 8-PSK @ 12.5k (37,500 bps)

#### Tier 3 - High (12,500 baud, variable envelope):
- `MODE_QAM16_12500`: 16-QAM @ 12.5k (50,000 bps / 50 kbps)
- `MODE_QAM64_12500`: 64-QAM @ 12.5k (75,000 bps / 75 kbps)
- `MODE_QAM256`: 256-QAM @ 12.5k (100,000 bps / 100 kbps)

#### Tier 4 - Broadband (23cm/13cm bands, SOQPSK constant envelope) - **DISABLED BY DEFAULT**:
- `MODE_SOQPSK_1M`: SOQPSK @ 781 kbaud (1 Mbps) - ~1 MHz bandwidth
- `MODE_SOQPSK_5M`: SOQPSK @ 3.9 Mbaud (5 Mbps) - ~5 MHz bandwidth
- `MODE_SOQPSK_10M`: SOQPSK @ 7.8 Mbaud (10 Mbps) - ~10 MHz bandwidth
- `MODE_SOQPSK_20M`: SOQPSK @ 15.6 Mbaud (20 Mbps) - ~20 MHz bandwidth
- `MODE_SOQPSK_40M`: SOQPSK @ 31.3 Mbaud (40 Mbps) - ~40 MHz bandwidth - **Top speed**

**IMPORTANT: Tier 4 modes are DISABLED BY DEFAULT to prevent accidental out-of-bandwidth transmissions on standard 12.5 kHz VHF/UHF channels. These modes require 1-40 MHz bandwidth and will cause severe interference if used on narrowband channels. They are ONLY suitable for broadband segments in 23cm (1.2 GHz) and 13cm (2.4 GHz) amateur radio bands with SDR hardware. Must be explicitly enabled via `enable_tier4=True` parameter.**

#### Legacy Modes (lower baud rates):
- `MODE_BPSK`: Binary PSK (1,200 bps)
- `MODE_QPSK`: Quadrature PSK (2,400 bps)
- `MODE_8PSK`: 8-PSK (3,600 bps)
- `MODE_QAM16`: 16-QAM (9,600 bps @ 2,400 baud)
- `MODE_QAM64_6250`: 64-QAM (37,500 bps @ 6,250 baud)

### Using GNU Radio Modulation Blocks

The adaptive rate control selects the mode, but you need to configure the actual modulation blocks:

#### For FSK Modulation

```python
from gnuradio import digital

# For 2FSK/4FSK, use GFSK or CPM modulator
# GFSK is suitable for binary FSK
gfsk_mod = digital.gfsk_mod(
    samples_per_symbol=2,
    sensitivity=1.0,
    bt=0.35
)

# For M-FSK (4FSK, 8FSK, 16FSK), use CPM modulator
from gnuradio.analog import cpm
cpm_mod = cpm.cpmmod_bc(
    type=cpm.CPM_LREC,  # or CPM_LRC, CPM_LSRC
    h=0.5,              # Modulation index
    samples_per_sym=2,
    L=4,                # Pulse length
    beta=0.3            # Rolloff factor
)
```

#### For PSK Modulation

```python
from gnuradio import digital

# BPSK
bpsk_mod = digital.psk.psk_mod(
    constellation_points=2,
    mod_code=digital.GRAY_CODE,
    differential=False
)

# QPSK
qpsk_mod = digital.psk.psk_mod(
    constellation_points=4,
    mod_code=digital.GRAY_CODE,
    differential=False
)

# 8PSK
psk8_mod = digital.psk.psk_mod(
    constellation_points=8,
    mod_code=digital.GRAY_CODE,
    differential=False
)
```

#### For QAM Modulation

```python
from gnuradio import digital

# 16-QAM
qam16_mod = digital.qam.qam_mod(
    constellation_points=16,
    mod_code=digital.GRAY_CODE,
    differential=False
)

# 64-QAM (12,500 baud × 6 = 75,000 bps)
qam64_mod = digital.qam.qam_mod(
    constellation_points=64,
    mod_code=digital.GRAY_CODE,
    differential=False
)

# 256-QAM (12,500 baud × 8 = 100,000 bps, highest rate)
qam256_mod = digital.qam.qam_mod(
    constellation_points=256,
    mod_code=digital.GRAY_CODE,
    differential=False
)
```

### Complete Example Flowgraph

```python
from gnuradio import gr, blocks, digital, packet_protocols
from gnuradio.analog import cpm

class adaptive_packet_tx(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        
        # Packet encoder
        encoder = packet_protocols.ax25_encoder(...)
        
        # Link quality monitor
        quality_monitor = packet_protocols.link_quality_monitor()
        
        # Adaptive rate control
        rate_control = packet_protocols.adaptive_rate_control(
            initial_mode=packet_protocols.modulation_mode_t.MODE_2FSK  # Default: Bell 202 / AX.25
        )
        
        # SNR estimator (from GNU Radio)
        snr_est = digital.probe_mpsk_snr_est_c(
            type=digital.SNR_EST_SIMPLE
        )
        
        # Modulation blocks (switch based on rate_control.get_modulation_mode())
        # This is simplified - in practice you'd use a selector or switch
        mod_4fsk = cpm.cpmmod_bc(...)
        
        # Connect blocks
        self.connect(encoder, quality_monitor, mod_4fsk)
        
        # Update rate control based on quality
        # (This would typically be done via message passing or callbacks)
```

## Modulation Negotiation Protocol

The modulation negotiation extends the KISS protocol with additional commands for inter-station communication.

### KISS Extension Commands

- `KISS_CMD_NEG_REQ (0x10)`: Negotiation request
- `KISS_CMD_NEG_RESP (0x11)`: Negotiation response
- `KISS_CMD_NEG_ACK (0x12)`: Negotiation acknowledgment
- `KISS_CMD_MODE_CHANGE (0x13)`: Mode change notification
- `KISS_CMD_QUALITY_FB (0x14)`: Quality feedback

### Basic Usage

```python
from gnuradio import packet_protocols
from gnuradio import pmt

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

# Create KISS TNC
tnc = packet_protocols.kiss_tnc(
    device="/dev/ttyUSB0",
    baud_rate=9600
)

# Connect message ports for negotiation frames
tnc.msg_connect(tnc, "negotiation_out", negotiator, "negotiation_in")

# Set callback for sending KISS frames
def send_kiss_frame(command, data):
    # Create KISS frame and send to TNC input
    frame = bytearray()
    frame.append(0xC0)  # KISS_FEND
    frame.append(command)
    frame.extend(data)
    frame.append(0xC0)  # KISS_FEND
    # Send frame to TNC (implementation depends on your flowgraph)

negotiator.set_kiss_frame_sender(send_kiss_frame)

# Initiate negotiation
negotiator.initiate_negotiation(
    remote_station_id="N1CALL",
    proposed_mode=packet_protocols.modulation_mode_t.MODE_2FSK  # Default: Bell 202 / AX.25
)

# Check negotiated mode
if not negotiator.is_negotiating():
    mode = negotiator.get_negotiated_mode()
```

### Automatic Negotiation

Enable automatic negotiation that triggers when the adaptive rate control changes modes:

```python
# Create adaptive rate control
rate_control = packet_protocols.adaptive_rate_control(
    initial_mode=packet_protocols.modulation_mode_t.MODE_2FSK,  # Default: Bell 202 / AX.25
    enable_adaptation=True
)

# Enable automatic negotiation
negotiator.set_auto_negotiation_enabled(
    enabled=True,
    rate_control=rate_control
)

# Now when rate_control changes mode, negotiation is automatically triggered
```

### Quality Feedback

Send quality feedback to remote stations:

```python
# Send quality metrics to remote station
negotiator.send_quality_feedback(
    remote_station_id="N1CALL",
    snr_db=15.5,
    ber=0.001,
    quality_score=0.85
)
```

### Frame Format Details

**Negotiation Request (0x10)**:
- Station ID length (1 byte)
- Station ID (variable length)
- Proposed mode (1 byte)
- Number of supported modes (1 byte)
- Supported modes list (1 byte each, max 8)

**Negotiation Response (0x11)**:
- Station ID length (1 byte)
- Station ID (variable length)
- Accepted flag (1 byte: 0=rejected, 1=accepted)
- Negotiated mode (1 byte)

**Mode Change (0x13)**:
- Station ID length (1 byte)
- Station ID (variable length)
- New mode (1 byte)

**Quality Feedback (0x14)**:
- Station ID length (1 byte)
- Station ID (variable length)
- SNR in dB (4 bytes, IEEE 754 float)
- BER (4 bytes, IEEE 754 float)
- Quality score (4 bytes, IEEE 754 float)

## Integration with KISS TNC

The adaptive features integrate with KISS TNC via message ports:

```python
from gnuradio import packet_protocols
from gnuradio import pmt

# Create KISS TNC
tnc = packet_protocols.kiss_tnc(
    device="/dev/ttyUSB0",
    baud_rate=9600
)

# Create adaptive components
quality_monitor = packet_protocols.link_quality_monitor()
rate_control = packet_protocols.adaptive_rate_control()
negotiator = packet_protocols.modulation_negotiation(
    station_id="N0CALL",
    supported_modes=[...]
)

# Connect negotiation message ports
tnc.msg_connect(tnc, "negotiation_out", negotiator, "negotiation_in")

# Set up callback for sending frames (see negotiation example above)
negotiator.set_kiss_frame_sender(send_kiss_frame_callback)

# Enable automatic negotiation
negotiator.set_auto_negotiation_enabled(True, rate_control)
```

The KISS TNC automatically forwards received negotiation frames (commands 0x10-0x14) to the `negotiation_out` message port, which can be connected to the modulation_negotiation block's `negotiation_in` port.

## Thresholds and Configuration

### Default Thresholds

Each modulation mode has default thresholds:

| Mode | Min SNR (dB) | Max SNR (dB) | Max BER | Min Quality | Data Rate |
|------|--------------|--------------|---------|-------------|-----------|
| **Tier 1 (1,200 baud)** ||||||
| 2FSK (Bell 202/AX.25) | 0.0 | 15.0 | 0.01 | 0.3 | 1,200 bps |
| 4FSK | 8.0 | 20.0 | 0.005 | 0.5 | 2,400 bps |
| 8FSK | 12.0 | 25.0 | 0.001 | 0.7 | 3,600 bps |
| 16FSK | 18.0 | 30.0 | 0.0005 | 0.8 | 4,800 bps |
| **Tier 2 (12,500 baud PSK)** ||||||
| BPSK @ 12.5k | 8.0 | 20.0 | 0.005 | 0.5 | 12,500 bps |
| QPSK @ 12.5k | 12.0 | 24.0 | 0.002 | 0.65 | 25,000 bps |
| 8PSK @ 12.5k | 16.0 | 28.0 | 0.0008 | 0.78 | 37,500 bps |
| **Tier 3 (12,500 baud QAM)** ||||||
| 16-QAM @ 12.5k | 18.0 | 30.0 | 0.0003 | 0.82 | 50,000 bps |
| 64-QAM @ 12.5k | 22.0 | 35.0 | 0.0001 | 0.9 | 75,000 bps |
| 256-QAM @ 12.5k | 28.0 | 40.0 | 0.00005 | 0.95 | 100,000 bps |
| **Legacy Modes** ||||||
| BPSK | 6.0 | 18.0 | 0.01 | 0.4 | 1,200 bps |
| QPSK | 10.0 | 22.0 | 0.005 | 0.6 | 2,400 bps |
| 8PSK | 14.0 | 26.0 | 0.001 | 0.75 | 3,600 bps |
| 16-QAM | 16.0 | 28.0 | 0.0005 | 0.8 | 9,600 bps |
| 64-QAM @ 6.25k | 20.0 | 32.0 | 0.0001 | 0.85 | 37,500 bps |
| **Tier 4 (Broadband SOQPSK)** ||||||
| SOQPSK 1M | 10.0 | 25.0 | 0.001 | 0.6 | 1,000,000 bps (1 Mbps) |
| SOQPSK 5M | 15.0 | 30.0 | 0.0005 | 0.7 | 5,000,000 bps (5 Mbps) |
| SOQPSK 10M | 18.0 | 33.0 | 0.0003 | 0.75 | 10,000,000 bps (10 Mbps) |
| SOQPSK 20M | 22.0 | 36.0 | 0.0002 | 0.8 | 20,000,000 bps (20 Mbps) |
| SOQPSK 40M | 26.0 | 40.0 | 0.0001 | 0.85 | 40,000,000 bps (40 Mbps) |

### Hysteresis

Hysteresis prevents rapid mode switching. The default is 2.0 dB, meaning:
- Switch to higher rate: SNR must exceed max threshold + 2.0 dB
- Switch to lower rate: SNR must fall below min threshold - 2.0 dB

## Future Enhancements

1. **Machine Learning Integration**: Use ML models to predict channel conditions
2. **Multi-Station Coordination**: Coordinate mode changes across multiple stations
3. **Dynamic Threshold Adjustment**: Learn optimal thresholds from link performance
4. **Advanced FEC Selection**: Automatically adjust FEC based on link quality

## References

- GNU Radio Digital Module: https://wiki.gnuradio.org/index.php/Digital
- GNU Radio Modulation Blocks: https://wiki.gnuradio.org/index.php/Modulation
- KISS Protocol Specification: http://www.ax25.net/kiss.aspx


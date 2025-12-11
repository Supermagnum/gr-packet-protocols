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
    initial_mode=packet_protocols.modulation_mode_t.MODE_4FSK,
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

Supported modulation modes:

- `MODE_2FSK`: Binary FSK (most robust, 1200 bps)
- `MODE_4FSK`: 4-level FSK (2400 bps)
- `MODE_8FSK`: 8-level FSK (3600 bps)
- `MODE_16FSK`: 16-level FSK (4800 bps)
- `MODE_BPSK`: Binary PSK (1200 bps)
- `MODE_QPSK`: Quadrature PSK (2400 bps)
- `MODE_8PSK`: 8-PSK (3600 bps)
- `MODE_QAM16`: 16-QAM (4800 bps)
- `MODE_QAM64`: 64-QAM (9600 bps)

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

# 64-QAM
qam64_mod = digital.qam.qam_mod(
    constellation_points=64,
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
            initial_mode=packet_protocols.modulation_mode_t.MODE_4FSK
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

The modulation negotiation extends the KISS protocol with additional commands:

### KISS Extension Commands

- `KISS_CMD_NEG_REQ (0x10)`: Negotiation request
- `KISS_CMD_NEG_RESP (0x11)`: Negotiation response
- `KISS_CMD_NEG_ACK (0x12)`: Negotiation acknowledgment
- `KISS_CMD_MODE_CHANGE (0x13)`: Mode change notification
- `KISS_CMD_QUALITY_FB (0x14)`: Quality feedback

### Usage

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

# Initiate negotiation
negotiator.initiate_negotiation(
    remote_station_id="N1CALL",
    proposed_mode=packet_protocols.modulation_mode_t.MODE_4FSK
)

# Check negotiated mode
if not negotiator.is_negotiating():
    mode = negotiator.get_negotiated_mode()
```

## Integration with KISS TNC

The adaptive features can be integrated into the KISS TNC block:

```python
from gnuradio import packet_protocols

# Create KISS TNC with adaptive features
tnc = packet_protocols.kiss_tnc(
    device="/dev/ttyUSB0",
    baud_rate=9600
)

# Enable adaptive features (when implemented)
tnc.set_adaptive_mode_enabled(True)
tnc.set_link_quality_monitor(quality_monitor)
tnc.set_rate_control(rate_control)
```

## Thresholds and Configuration

### Default Thresholds

Each modulation mode has default thresholds:

| Mode | Min SNR (dB) | Max SNR (dB) | Max BER | Min Quality |
|------|--------------|--------------|---------|-------------|
| 2FSK | 0.0 | 15.0 | 0.01 | 0.3 |
| 4FSK | 8.0 | 20.0 | 0.005 | 0.5 |
| 8FSK | 12.0 | 25.0 | 0.001 | 0.7 |
| 16FSK | 18.0 | 30.0 | 0.0005 | 0.8 |
| BPSK | 6.0 | 18.0 | 0.01 | 0.4 |
| QPSK | 10.0 | 22.0 | 0.005 | 0.6 |
| 8PSK | 14.0 | 26.0 | 0.001 | 0.75 |
| 16-QAM | 16.0 | 28.0 | 0.0005 | 0.8 |
| 64-QAM | 22.0 | 35.0 | 0.0001 | 0.9 |

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


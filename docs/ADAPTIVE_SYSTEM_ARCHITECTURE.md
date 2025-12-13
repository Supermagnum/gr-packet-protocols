# Adaptive System Architecture

This document explains how the adaptive rate control, link quality monitor, and modulation negotiation components work together.

## Overview

The adaptive modulation system has **two levels of operation**:

1. **Local Adaptation** - Each station independently measures channel quality and adapts its transmission mode
2. **Inter-Station Negotiation** - Stations communicate to agree on a common modulation mode

## Component Architecture

### 1. Link Quality Monitor (Local Only)

**Purpose**: Monitors channel quality metrics at the local station.

**How it works**:
- Receives SNR measurements from GNU Radio blocks (e.g., `probe_mpsk_snr_est_c`)
- Receives BER measurements from error detection blocks
- Tracks frame error rate (FER) by counting successful/failed frames
- Calculates a composite quality score (0.0-1.0)

**Key Point**: This is **purely local** - it does NOT communicate with remote stations. It only measures the quality of signals received at THIS station.

**Data Flow**:
```
[Receive Path] → [SNR Estimator] → quality_monitor.update_snr(snr_db)
[Receive Path] → [BER Detector] → quality_monitor.update_ber(ber)
[Frame Decoder] → quality_monitor.record_frame_success() or record_frame_error()
```

**Implementation Details**:
- Uses exponential moving average (EMA) for smoothing
- Maintains history of SNR/BER measurements
- Updates quality score periodically based on all metrics

### 2. Adaptive Rate Control (Local Decision Making)

**Purpose**: Automatically selects the best modulation mode based on local quality measurements.

**How it works**:
- Receives quality metrics from Link Quality Monitor (via function calls)
- Compares metrics against thresholds for each modulation mode
- Selects highest-rate mode that meets quality requirements
- Uses hysteresis to prevent rapid mode switching

**Key Point**: This makes **local decisions** only. It doesn't know what the remote station is using or what it supports.

**Data Flow**:
```
quality_monitor.get_snr() → rate_control.update_quality(snr, ber, quality_score)
quality_monitor.get_ber() → rate_control.update_quality(snr, ber, quality_score)
quality_monitor.get_quality_score() → rate_control.update_quality(snr, ber, quality_score)
                                    ↓
                          rate_control.get_modulation_mode() → [Modulation Switch]
```

**Decision Logic**:
1. If SNR > (current_mode_max + hysteresis) AND BER < threshold → Try higher rate mode
2. If SNR < (current_mode_min - hysteresis) OR BER > threshold → Switch to lower rate mode
3. Select highest-rate mode that meets all thresholds

**Example**:
- Current mode: 4FSK (requires 8-20 dB SNR)
- Measured SNR: 25 dB, BER: 0.0005
- Decision: Switch to 8FSK (requires 12-25 dB SNR) for higher data rate

### 3. Modulation Negotiation (Inter-Station Communication)

**Purpose**: Allows two stations to agree on a common modulation mode.

**How it works** (as designed):
- Station A proposes a modulation mode to Station B
- Station B responds with acceptance/rejection or counter-proposal
- Both stations switch to agreed mode
- Stations can send quality feedback to each other

**Key Point**: This is where **inter-station communication** happens! The implementation includes full KISS protocol extensions and frame encoding/decoding.

**Current Status**:
- Basic structure exists
- Functions for initiating negotiation are implemented
- **Implemented**: KISS protocol extensions (commands 0x10-0x14) for sending negotiation frames over the air
- **Implemented**: Frame parsing to receive and decode negotiation messages

**Data Flow** (fully implemented):
```
Station A: negotiator.initiate_negotiation("N1CALL", MODE_8FSK)
           ↓
           [Creates KISS negotiation frame via negotiation_frame.cc]
           ↓
           [Sends over radio via KISS TNC using callback]
           ↓
Station B: [Receives frame via KISS TNC]
           ↓
           [KISS TNC forwards to negotiation_out message port]
           ↓
           [Modulation Negotiation receives via negotiation_in port]
           ↓
           negotiator.handle_negotiation_frame()
           ↓
           [Sends response frame]
           ↓
Station A: [Receives response via message port]
           ↓
           [Both stations switch to agreed mode (automatic if enabled)]
```

**KISS Protocol Extensions** (implemented):
- `KISS_CMD_NEG_REQ (0x10)`: Negotiation request frame
- `KISS_CMD_NEG_RESP (0x11)`: Negotiation response frame
- `KISS_CMD_NEG_ACK (0x12)`: Negotiation acknowledgment
- `KISS_CMD_MODE_CHANGE (0x13)`: Mode change notification
- `KISS_CMD_QUALITY_FB (0x14)`: Quality feedback message

**Message Port Integration**:
- KISS TNC has `negotiation_out` message port that forwards received negotiation frames
- Modulation Negotiation has `negotiation_in` message port for receiving frames
- Connect: `tnc.msg_connect(tnc, "negotiation_out", negotiator, "negotiation_in")`

## How They Work Together

### Scenario 1: Local Adaptation Only (Current Implementation)

```
┌─────────────────────────────────────────────────────────┐
│                    Station A (Local)                      │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  [Receive Signal] → [SNR Estimator]                      │
│                          ↓                                │
│                  [Link Quality Monitor]                   │
│                          ↓                                │
│              [Adaptive Rate Control]                      │
│                          ↓                                │
│              [Modulation Switch] → [Transmit]            │
│                                                           │
└─────────────────────────────────────────────────────────┘
                          │
                          │ Radio Link
                          │
┌─────────────────────────────────────────────────────────┐
│                    Station B (Remote)                     │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  [Receive Signal] → [SNR Estimator]                      │
│                          ↓                                │
│                  [Link Quality Monitor]                  │
│                          ↓                                │
│              [Adaptive Rate Control]                     │
│                          ↓                                │
│              [Modulation Switch] → [Transmit]            │
│                                                           │
└─────────────────────────────────────────────────────────┘
```

**What happens**:
1. Station A measures its receive quality (from Station B's transmissions)
2. Station A adapts its **transmit** mode based on what it receives
3. Station B measures its receive quality (from Station A's transmissions)
4. Station B adapts its **transmit** mode based on what it receives
5. **Problem**: Stations may end up using different modes!

**Limitation**: Each station adapts independently, so they might not agree on a mode.

### Scenario 2: With Negotiation (Fully Implemented)

```
┌─────────────────────────────────────────────────────────┐
│                    Station A                             │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  [Link Quality Monitor] → [Adaptive Rate Control]       │
│           ↓                        ↓                     │
│    [Modulation Negotiation] ← [Quality Feedback]        │
│           ↓                                              │
│    [KISS TNC] → [Radio] → [Station B]                  │
│                                                           │
└─────────────────────────────────────────────────────────┘
                          │
                          │ Negotiation Frames
                          │
┌─────────────────────────────────────────────────────────┐
│                    Station B                             │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  [Link Quality Monitor] → [Adaptive Rate Control]       │
│           ↓                        ↓                     │
│    [Modulation Negotiation] ← [Quality Feedback]        │
│           ↓                                              │
│    [KISS TNC] → [Radio] → [Station A]                  │
│                                                           │
└─────────────────────────────────────────────────────────┘
```

**What happens**:
1. Station A measures quality and decides it wants to use MODE_8FSK
2. Station A sends negotiation request to Station B
3. Station B checks if it supports MODE_8FSK
4. Station B responds with acceptance
5. Both stations switch to MODE_8FSK simultaneously
6. Stations periodically exchange quality feedback

**Advantage**: Both stations use the same mode, ensuring compatibility.

## Channel Quality Measurement

### How SNR is Measured

**At the receiving station**:
1. GNU Radio's `probe_mpsk_snr_est_c` block estimates SNR from the received signal
2. SNR value is passed to `link_quality_monitor.update_snr(snr_db)`
3. Link Quality Monitor applies exponential smoothing
4. Adaptive Rate Control uses smoothed SNR for decisions

**Key Point**: SNR is measured from **received signals**, not transmitted signals. Each station measures the quality of what it **receives** from the other station.

### How BER is Measured

**Methods**:
1. **Known data comparison**: Compare received data with known transmitted data
2. **FEC decoding errors**: Count errors corrected by forward error correction
3. **Frame validation**: Count frames that fail CRC/FCS checks

**Implementation**:
- BER can be estimated from frame error rate
- Or measured directly if known data is available
- Passed to `link_quality_monitor.update_ber(ber)`

### How FER is Measured

**Automatic**:
- `link_quality_monitor.record_frame_success()` - called when frame passes validation
- `link_quality_monitor.record_frame_error()` - called when frame fails validation
- FER = error_frames / total_frames

## Current Implementation Status

### Fully Implemented

1. **Link Quality Monitor**
   - SNR tracking with exponential moving average (EMA)
   - BER tracking with EMA
   - FER calculation from frame success/error counts
   - Quality score computation (weighted combination of SNR, BER, FER)
   - Thread-safe accessors for all metrics
   - History maintenance for trend analysis

2. **Adaptive Rate Control**
   - Mode recommendation based on SNR/BER thresholds
   - Hysteresis to prevent rapid switching (configurable dB margin)
   - Automatic mode switching logic
   - Data rate lookup for each modulation mode
   - Support for 9 modulation modes (2FSK through 64-QAM)
   - Threshold-based mode selection algorithm

### Fully Implemented

3. **Modulation Negotiation**
   - Complete class structure and API
   - Functions for initiating negotiation (`initiate_negotiation()`)
   - Station ID tracking and supported modes management
   - Negotiation timeout handling
   - Mode storage per remote station
   - KISS protocol command extensions (0x10-0x14) implemented
   - Frame encoding/decoding for all negotiation message types
   - Integration with KISS TNC via message ports
   - Message parsing from received KISS frames
   - Automatic negotiation triggers when mode changes
   - Quality feedback message transmission
   - Bidirectional communication between stations

## Usage Patterns

### Pattern 1: Local Adaptation (Works Now)

```python
# Station A
quality_monitor = packet_protocols.link_quality_monitor()
rate_control = packet_protocols.adaptive_rate_control()

# Periodically update (from SNR estimator, etc.)
quality_monitor.update_snr(measured_snr)
quality_monitor.update_ber(measured_ber)
quality_score = quality_monitor.get_quality_score()

# Update rate control
rate_control.update_quality(
    quality_monitor.get_snr(),
    quality_monitor.get_ber(),
    quality_score
)

# Get current mode
current_mode = rate_control.get_modulation_mode()
# Use this to select modulation block
```

**Result**: Station adapts locally, but remote station may use different mode.

### Pattern 2: With Negotiation (Fully Implemented)

```python
# Station A
negotiator = packet_protocols.modulation_negotiation(
    station_id="N0CALL",
    supported_modes=[MODE_2FSK, MODE_4FSK, MODE_8FSK]
)

# Measure quality and decide on mode
recommended = rate_control.recommend_mode(snr, ber)

# Negotiate with remote station
negotiator.initiate_negotiation("N1CALL", recommended)

# Wait for response
while negotiator.is_negotiating():
    time.sleep(0.1)

# Get agreed mode
agreed_mode = negotiator.get_negotiated_mode()
rate_control.set_modulation_mode(agreed_mode)
```

**Result**: Both stations use the same mode.

## Key Insights

1. **Quality measurement is local**: Each station measures the quality of signals it receives
2. **Adaptation is local**: Each station independently decides what mode to use
3. **Negotiation is inter-station**: Requires communication over the radio link (fully implemented)
4. **With negotiation enabled**: Stations coordinate to use compatible modes
5. **Implementation complete**: Full negotiation implementation ensures mode compatibility

## Questions Answered

**Q: Is there communication between modem instances to measure channel quality?**

**A**: Quality measurement itself is **local** - each station measures what it receives. The **negotiation protocol** (fully implemented) allows stations to:
- Exchange quality feedback
- Agree on common modulation modes
- Coordinate mode changes

**Q: How does a station know what mode the remote station is using?**

**A**: With negotiation enabled, stations know each other's modes! The negotiation protocol (fully implemented) solves this by:
- Sending mode change notifications
- Exchanging supported mode lists
- Confirming mode switches

**Q: Can stations use different modes?**

**A**: Without negotiation, yes - stations can use different modes. Station A might transmit in 8FSK while Station B transmits in 4FSK. This works if both stations can receive and decode the other's mode. With negotiation enabled, they will coordinate to use the same mode for optimal compatibility.

## Implementation Details

### KISS Protocol Extensions

The following KISS commands are implemented for negotiation:

- **KISS_CMD_NEG_REQ (0x10)**: Negotiation request frame
  - Contains: station ID, proposed mode, list of supported modes
  - Sent when initiating negotiation

- **KISS_CMD_NEG_RESP (0x11)**: Negotiation response frame
  - Contains: station ID, acceptance flag, negotiated mode
  - Sent in response to negotiation request

- **KISS_CMD_NEG_ACK (0x12)**: Negotiation acknowledgment
  - Contains: station ID, confirmed mode
  - Sent to confirm mode change

- **KISS_CMD_MODE_CHANGE (0x13)**: Mode change notification
  - Contains: station ID, new mode
  - Sent automatically when mode changes (if auto-negotiation enabled)

- **KISS_CMD_QUALITY_FB (0x14)**: Quality feedback message
  - Contains: station ID, SNR (dB), BER, quality score
  - Sent periodically or on request

### Frame Format

All negotiation frames follow a consistent format:
- Station ID length (1 byte) + Station ID (variable)
- Frame-specific data (varies by command type)

Quality feedback frames include IEEE 754 float values for SNR, BER, and quality score.

### Message Port Integration

- **KISS TNC** has message port `negotiation_out` for forwarding received negotiation frames
- **Modulation Negotiation** has message port `negotiation_in` for receiving frames
- Connect them: `tnc.msg_connect(tnc, "negotiation_out", negotiator, "negotiation_in")`

### Automatic Negotiation

When enabled via `set_auto_negotiation_enabled(True, rate_control)`:
- Monitors adaptive_rate_control for mode changes
- Automatically sends mode change notifications
- Initiates negotiation with active remote stations
- Sends notifications to all known remote stations

## Future Enhancements

1. **Periodic re-negotiation**: Automatic re-negotiation based on quality changes
2. **Fallback mechanisms**: Automatic fallback to safe modes on negotiation failure
3. **Multi-station coordination**: Coordinate mode changes across multiple stations simultaneously
4. **Quality-based negotiation**: Use quality feedback to guide negotiation decisions


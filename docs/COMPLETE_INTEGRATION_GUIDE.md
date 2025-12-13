# Complete Integration Guide: Adaptive Modulation System

This guide provides step-by-step instructions for integrating the adaptive modulation system into your GNU Radio flowgraph.

## Overview

The adaptive modulation system consists of:
1. **Link Quality Monitor** - Monitors SNR, BER, and frame error rate
2. **Adaptive Rate Control** - Automatically selects optimal modulation mode
3. **Modulation Blocks** - Actual GNU Radio modulation blocks (FSK, PSK, QAM)
4. **Selector Block** - Switches between modulation paths

## Step-by-Step Integration

### Step 1: Import Required Modules

```python
from gnuradio import gr, blocks, digital
from gnuradio.analog import cpm
from gnuradio import packet_protocols
```

### Step 2: Create Link Quality Monitor

```python
# Create link quality monitor
quality_monitor = packet_protocols.link_quality_monitor(
    alpha=0.1,        # Smoothing factor (0.0-1.0, higher = faster response)
    update_period=1000  # Update every N samples
)
```

### Step 3: Create Adaptive Rate Control

```python
# Create adaptive rate controller
rate_control = packet_protocols.adaptive_rate_control(
    initial_mode=packet_protocols.modulation_mode_t.MODE_4FSK,
    enable_adaptation=True,   # Enable automatic mode switching
    hysteresis_db=2.0        # Prevent rapid switching (2 dB margin)
)
```

### Step 4: Create Modulation Blocks

Create modulation blocks for each mode you want to support:

```python
samples_per_symbol = 2

# 2FSK - Most robust, lowest rate
mod_2fsk = digital.gfsk_mod(
    samples_per_symbol=samples_per_symbol,
    sensitivity=1.0,
    bt=0.35
)

# 4FSK - Good balance
mod_4fsk = cpm.cpmmod_bc(
    type=cpm.CPM_LREC,
    h=0.5,
    samples_per_sym=samples_per_symbol,
    L=4,
    beta=0.3
)

# 8FSK - Higher rate
mod_8fsk = cpm.cpmmod_bc(
    type=cpm.CPM_LREC,
    h=0.5,
    samples_per_sym=samples_per_symbol,
    L=4,
    beta=0.3
)

# QPSK - Good PSK option
mod_qpsk = digital.psk.psk_mod(
    constellation_points=4,
    mod_code=digital.GRAY_CODE,
    differential=False
)

# 16-QAM - High rate
mod_qam16 = digital.qam.qam_mod(
    constellation_points=16,
    mod_code=digital.GRAY_CODE,
    differential=False
)
```

### Step 5: Create Modulation Switch Block

Use the custom `modulation_switch` block for better control:

```python
from gnuradio.packet_protocols import modulation_switch

# Create mode to index mapping
mode_to_index = {
    packet_protocols.modulation_mode_t.MODE_2FSK: 0,
    packet_protocols.modulation_mode_t.MODE_4FSK: 1,
    packet_protocols.modulation_mode_t.MODE_8FSK: 2,
    packet_protocols.modulation_mode_t.MODE_QPSK: 3,
    packet_protocols.modulation_mode_t.MODE_QAM16: 4,
}

# Create modulation switch block
mod_switch = modulation_switch(
    rate_control,
    mode_to_index,
    num_inputs=5  # Number of modulation modes
)
```

**Alternative: Use GNU Radio Selector**

```python
# Create selector to switch between modulators
# Start with initial mode (e.g., 4FSK = index 1)
initial_index = mode_to_index[packet_protocols.modulation_mode_t.MODE_4FSK]
selector = blocks.selector(gr.sizeof_gr_complex, initial_index)
```

### Step 6: Connect Blocks

```python
# Connect data path
# Source -> Encoder -> Quality Monitor -> All Modulators -> Selector -> Output

# Connect encoder to quality monitor
self.connect(encoder, quality_monitor)

# Connect quality monitor output to ALL modulators (they all receive same input)
self.connect((quality_monitor, 0), (mod_2fsk, 0))
self.connect((quality_monitor, 0), (mod_4fsk, 0))
self.connect((quality_monitor, 0), (mod_8fsk, 0))
self.connect((quality_monitor, 0), (mod_qpsk, 0))
self.connect((quality_monitor, 0), (mod_qam16, 0))

# Connect all modulators to modulation switch inputs
self.connect((mod_2fsk, 0), (mod_switch, 0))
self.connect((mod_4fsk, 0), (mod_switch, 1))
self.connect((mod_8fsk, 0), (mod_switch, 2))
self.connect((mod_qpsk, 0), (mod_switch, 3))
self.connect((mod_qam16, 0), (mod_switch, 4))

# Connect modulation switch output to your sink (SDR, file, etc.)
self.connect((mod_switch, 0), sink)
```

### Step 7: Setup Quality Monitoring

You need to feed quality metrics to the system. There are several approaches:

#### Option A: Use GNU Radio SNR Estimator

```python
from gnuradio import digital

# Create SNR estimator (place in receive path)
snr_estimator = digital.probe_mpsk_snr_est_c(
    type=digital.SNR_EST_SIMPLE,
    msg_nsamples=1000,
    alpha=0.001
)

# Connect to your receive signal path
self.connect(receive_signal, snr_estimator)

# Setup message handler to update quality monitor
def handle_snr(msg):
    snr_db = pmt.to_python(msg)
    quality_monitor.update_snr(snr_db)

snr_estimator.message_port_register_out(pmt.intern("snr"))
snr_estimator.msg_connect(snr_estimator, "snr", self, pmt.intern("snr_update"))
self.msg_connect(self, pmt.intern("snr_update"), handle_snr)
```

#### Option B: Manual Quality Updates

```python
import threading
import time

def update_quality_periodically():
    while True:
        time.sleep(1.0)  # Update every second
        
        # Get quality metrics from your system
        snr_db = get_snr_from_your_system()  # Your function
        ber = get_ber_from_your_system()     # Your function
        
        # Update link quality monitor
        quality_monitor.update_snr(snr_db)
        quality_monitor.update_ber(ber)
        
        # Get quality score
        quality_score = quality_monitor.get_quality_score()
        
        # Update adaptive rate control
        rate_control.update_quality(snr_db, ber, quality_score)
        
        # Send mode update message to modulation switch
        from gnuradio import pmt
        mod_switch.message_port_pub(
            pmt.intern("mode_update"),
            pmt.PMT_NIL
        )

# Start update thread
quality_thread = threading.Thread(target=update_quality_periodically, daemon=True)
quality_thread.start()
```

### Step 8: Handle Mode Changes

The selector block needs to be updated when the mode changes. Here are options:

#### Option A: Message-Based Mode Changes

```python
from gnuradio import pmt
import threading

class mode_change_handler(gr.sync_block):
    """Custom block to handle mode changes"""
    def __init__(self, rate_control, selector, mode_to_index):
        gr.sync_block.__init__(self, "mode_change_handler",
                               gr.io_signature(0, 0, 0),
                               gr.io_signature(0, 0, 0))
        self.rate_control = rate_control
        self.selector = selector
        self.mode_to_index = mode_to_index
        self.last_mode = None
        self.message_port_register_in(pmt.intern("mode_check"))
        self.set_msg_handler(pmt.intern("mode_check"), self.handle_mode_check)
        
    def handle_mode_check(self, msg):
        current_mode = self.rate_control.get_modulation_mode()
        if current_mode != self.last_mode and current_mode in self.mode_to_index:
            index = self.mode_to_index[current_mode]
            # Update selector
            if hasattr(self.selector, 'set_input_index'):
                self.selector.set_input_index(index)
            self.last_mode = current_mode

# Create mode change handler
mode_handler = mode_change_handler(rate_control, selector, mode_to_index)

# Periodically send mode check messages
def send_mode_checks():
    while True:
        time.sleep(0.1)  # Check every 100ms
        mode_handler.message_port_pub(pmt.intern("mode_check"), pmt.PMT_NIL)

threading.Thread(target=send_mode_checks, daemon=True).start()
```

#### Option B: Direct Selector Updates

If your GNU Radio version supports it:

```python
def update_selector_mode():
    last_mode = None
    while True:
        time.sleep(0.1)
        current_mode = rate_control.get_modulation_mode()
        if current_mode != last_mode and current_mode in mode_to_index:
            index = mode_to_index[current_mode]
            try:
                selector.set_input_index(index)
                last_mode = current_mode
            except AttributeError:
                # Selector doesn't support set_input_index
                # Use message-based approach instead
                pass

threading.Thread(target=update_selector_mode, daemon=True).start()
```

## Complete Example

See `examples/adaptive_modulation_example.py` for a complete working example.

## Block Diagram

```
[Data Source] 
    |
    v
[Packet Encoder (AX.25/FX.25/IL2P)]
    |
    v
[Link Quality Monitor]
    |
    +---> [2FSK Modulator] ----+
    |                          |
    +---> [4FSK Modulator] ----+
    |                          |
    +---> [8FSK Modulator] ----+---> [Selector] ---> [Output]
    |                          |
    +---> [QPSK Modulator] ----+
    |                          |
    +---> [16-QAM Modulator] --+
                               |
                               v
                        [Modulation Switch] ---> [Output]
    
[SNR Estimator] ---> [Quality Monitor] ---> [Adaptive Rate Control] ---> [Modulation Switch]
                                                      (via messages)
```

## Key Points

1. **All modulators run simultaneously** - The selector just chooses which output to use
2. **Quality updates drive adaptation** - Feed SNR/BER metrics regularly
3. **Mode switching is automatic** - When enabled, rate_control.update_quality() triggers mode changes
4. **Manual control available** - Call rate_control.set_modulation_mode() to override

## Troubleshooting

### Mode not switching
- Check that `enable_adaptation=True` in adaptive_rate_control
- Verify quality metrics are being updated regularly
- Check that selector input index is being updated when mode changes

### Poor performance
- Adjust `hysteresis_db` to prevent rapid switching
- Tune `alpha` in link_quality_monitor for appropriate smoothing
- Verify SNR/BER measurements are accurate

### Selector not updating
- Use message-based approach if `set_input_index()` not available
- Check GNU Radio version compatibility
- Consider using a custom switching block

## Advanced: Custom Switching Block

For more control, create a custom block:

```python
from gnuradio import gr

class modulation_switch(gr.sync_block):
    def __init__(self, rate_control, mode_to_index):
        gr.sync_block.__init__(self, "modulation_switch",
                               gr.io_signature(5, 5, gr.sizeof_gr_complex),  # 5 inputs
                               gr.io_signature(1, 1, gr.sizeof_gr_complex))  # 1 output
        self.rate_control = rate_control
        self.mode_to_index = mode_to_index
        self.last_mode = None
        
    def work(self, input_items, output_items):
        current_mode = self.rate_control.get_modulation_mode()
        if current_mode in self.mode_to_index:
            index = self.mode_to_index[current_mode]
            # Copy from selected input to output
            n = min(len(input_items[index]), len(output_items[0]))
            output_items[0][:n] = input_items[index][:n]
            return n
        return 0
```

## Step 9: Add Modulation Negotiation (Optional)

For inter-station coordination, add modulation negotiation:

```python
from gnuradio import packet_protocols

# Create modulation negotiator
negotiator = packet_protocols.modulation_negotiation(
    station_id="N0CALL",
    supported_modes=[
        packet_protocols.modulation_mode_t.MODE_2FSK,
        packet_protocols.modulation_mode_t.MODE_4FSK,
        packet_protocols.modulation_mode_t.MODE_8FSK,
        packet_protocols.modulation_mode_t.MODE_QPSK
    ],
    negotiation_timeout_ms=5000
)

# If using KISS TNC, connect message ports
# tnc.msg_connect(tnc, "negotiation_out", negotiator, "negotiation_in")

# Set callback for sending KISS frames
def send_kiss_callback(command, data):
    # Implementation depends on your setup
    # This should send the KISS frame to your TNC or radio interface
    pass

negotiator.set_kiss_frame_sender(send_kiss_callback)

# Enable automatic negotiation
negotiator.set_auto_negotiation_enabled(True, rate_control)

# Manually initiate negotiation if needed
negotiator.initiate_negotiation("N1CALL", packet_protocols.modulation_mode_t.MODE_4FSK)
```

### Quality Feedback

Periodically send quality feedback to remote stations:

```python
import threading
import time

def send_periodic_feedback():
    while True:
        time.sleep(5.0)  # Every 5 seconds
        snr = quality_monitor.get_snr()
        ber = quality_monitor.get_ber()
        quality = quality_monitor.get_quality_score()
        
        negotiator.send_quality_feedback("N1CALL", snr, ber, quality)

threading.Thread(target=send_periodic_feedback, daemon=True).start()
```

## See Also

- [Adaptive Features Guide](ADAPTIVE_FEATURES.md) - Feature overview
- [Modulation Control](MODULATION_CONTROL.md) - Mode control details
- [Adaptive System Architecture](ADAPTIVE_SYSTEM_ARCHITECTURE.md) - Detailed architecture
- [Example Code](../examples/adaptive_modulation_example.py) - Complete working example


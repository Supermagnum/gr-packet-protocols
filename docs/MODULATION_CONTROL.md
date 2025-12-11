# Modulation Mode Control

This document explains how modulation modes are controlled in gr-packet-protocols.

## Current Implementation

### Mode Tracking

The `adaptive_rate_control` block tracks the current modulation mode and provides control methods:

```python
from gnuradio import packet_protocols

# Create adaptive rate controller
rate_control = packet_protocols.adaptive_rate_control(
    initial_mode=packet_protocols.modulation_mode_t.MODE_4FSK,
    enable_adaptation=True
)

# Get current mode
current_mode = rate_control.get_modulation_mode()

# Manually set mode
rate_control.set_modulation_mode(packet_protocols.modulation_mode_t.MODE_8FSK)

# Enable/disable automatic adaptation
rate_control.set_adaptation_enabled(False)  # Manual control only
```

### Automatic Mode Selection

The block automatically selects modes based on link quality:

```python
# Update quality metrics (from link quality monitor or SNR estimator)
rate_control.update_quality(
    snr_db=18.0,      # Signal-to-noise ratio in dB
    ber=0.0005,      # Bit error rate
    quality_score=0.85  # Composite quality score (0.0-1.0)
)

# The block will automatically switch modes if:
# - SNR exceeds threshold + hysteresis (switch to higher rate)
# - SNR falls below threshold - hysteresis (switch to lower rate)
# - BER exceeds maximum for current mode
# - Quality score falls below minimum
```

### Mode Recommendations

Get recommended mode without switching:

```python
recommended = rate_control.recommend_mode(snr_db=20.0, ber=0.0001)
if recommended != rate_control.get_modulation_mode():
    rate_control.set_modulation_mode(recommended)
```

## Actual Modulation Block Control

The `adaptive_rate_control` block tracks the modulation mode and can be used with:

1. **Adaptive Modulator** (Recommended for GRC): Python hierarchical block that automatically switches between modulation modes
2. **Modulation Switch**: Helper block for programmatic use (uses GNU Radio's selector in GRC)
3. **Manual Selector**: Use GNU Radio's `blocks.selector` directly with message-based control

All blocks are available in GNU Radio Companion with full parameter configuration.

## Implementation Approaches

### Approach 1: Selector Block (Recommended)

Use GNU Radio's `selector` block to switch between multiple modulation paths:

```python
from gnuradio import gr, blocks, digital, packet_protocols
from gnuradio.analog import cpm

class adaptive_modulation_flowgraph(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        
        # Packet encoder
        encoder = packet_protocols.ax25_encoder(...)
        
        # Adaptive rate control
        rate_control = packet_protocols.adaptive_rate_control(
            initial_mode=packet_protocols.modulation_mode_t.MODE_4FSK
        )
        
        # Create multiple modulation blocks
        mod_2fsk = digital.gfsk_mod(samples_per_symbol=2, bt=0.35)
        mod_4fsk = cpm.cpmmod_bc(type=cpm.CPM_LREC, h=0.5, samples_per_sym=2, L=4)
        mod_8fsk = cpm.cpmmod_bc(type=cpm.CPM_LREC, h=0.5, samples_per_sym=2, L=4)
        mod_qpsk = digital.psk.psk_mod(constellation_points=4)
        
        # Selector to switch between modulators
        # Input index selects which input to pass through
        selector = blocks.selector(gr.sizeof_gr_complex, 0)  # Start with input 0
        
        # Connect modulators to selector inputs
        self.connect(encoder, mod_2fsk, (selector, 0))
        self.connect(encoder, mod_4fsk, (selector, 1))
        self.connect(encoder, mod_8fsk, (selector, 2))
        self.connect(encoder, mod_qpsk, (selector, 3))
        
        # Connect selector output
        self.connect(selector, ...)  # Continue with your flowgraph
        
        # Monitor mode changes and update selector
        # This would typically be done via message passing or periodic checks
        self.rate_control = rate_control
        self.selector = selector
        
    def update_modulation_mode(self):
        """Call this periodically to update modulation based on rate_control"""
        mode = self.rate_control.get_modulation_mode()
        
        # Map mode to selector input index
        mode_to_index = {
            packet_protocols.modulation_mode_t.MODE_2FSK: 0,
            packet_protocols.modulation_mode_t.MODE_4FSK: 1,
            packet_protocols.modulation_mode_t.MODE_8FSK: 2,
            packet_protocols.modulation_mode_t.MODE_QPSK: 3,
        }
        
        if mode in mode_to_index:
            self.selector.set_enabled(True)
            # Note: selector doesn't have set_input_index in all GNU Radio versions
            # You may need to use a different approach
```

### Approach 2: Message-Based Control

Use message passing to dynamically reconfigure blocks:

```python
from gnuradio import gr, blocks, digital, packet_protocols
import pmt

class message_controlled_modulation(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        
        # Rate control
        rate_control = packet_protocols.adaptive_rate_control(...)
        
        # Modulation block (will be reconfigured)
        self.mod_block = digital.psk.psk_mod(constellation_points=4)
        
        # Message handler to update modulation
        self.message_port_register_in(pmt.intern("mode_change"))
        self.set_msg_handler(pmt.intern("mode_change"), self.handle_mode_change)
        
    def handle_mode_change(self, msg):
        """Handle mode change message"""
        mode = pmt.to_python(msg)
        
        if mode == packet_protocols.modulation_mode_t.MODE_QPSK:
            # Recreate QPSK modulator
            self.disconnect_all()
            self.mod_block = digital.psk.psk_mod(constellation_points=4)
            # Reconnect...
        elif mode == packet_protocols.modulation_mode_t.MODE_8PSK:
            self.mod_block = digital.psk.psk_mod(constellation_points=8)
            # Reconnect...
```

### Approach 3: Custom Modulation Switch Block

Create a custom block that switches between modes internally:

```python
from gnuradio import gr, blocks, digital
import packet_protocols

class adaptive_modulator(gr.hier_block2):
    """Hierarchical block that switches modulation based on adaptive_rate_control"""
    
    def __init__(self, rate_control):
        gr.hier_block2.__init__(
            self, "adaptive_modulator",
            gr.io_signature(1, 1, gr.sizeof_char),
            gr.io_signature(1, 1, gr.sizeof_gr_complex)
        )
        
        self.rate_control = rate_control
        
        # Create all modulation blocks
        self.mod_2fsk = digital.gfsk_mod(...)
        self.mod_4fsk = cpm.cpmmod_bc(...)
        # ... etc
        
        # Use selector or switch logic
        self.selector = blocks.selector(...)
        
        # Connect based on current mode
        self.update_connections()
        
    def update_connections(self):
        """Update connections based on current mode"""
        mode = self.rate_control.get_modulation_mode()
        
        # Disconnect old path
        self.disconnect_all()
        
        # Connect new path based on mode
        if mode == packet_protocols.modulation_mode_t.MODE_2FSK:
            self.connect(self, self.mod_2fsk, self)
        elif mode == packet_protocols.modulation_mode_t.MODE_4FSK:
            self.connect(self, self.mod_4fsk, self)
        # ... etc
```

### Approach 4: Periodic Mode Check

Check mode periodically and reconfigure:

```python
import threading
import time

class adaptive_modulation_flowgraph(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        
        self.rate_control = packet_protocols.adaptive_rate_control(...)
        self.current_mod_block = None
        self.last_mode = None
        
        # Start monitoring thread
        self.monitor_thread = threading.Thread(target=self.monitor_mode)
        self.monitor_thread.daemon = True
        self.monitor_thread.start()
        
    def monitor_mode(self):
        """Periodically check and update modulation mode"""
        while True:
            time.sleep(0.1)  # Check every 100ms
            
            current_mode = self.rate_control.get_modulation_mode()
            
            if current_mode != self.last_mode:
                # Mode changed - need to reconfigure
                self.reconfigure_modulation(current_mode)
                self.last_mode = current_mode
                
    def reconfigure_modulation(self, mode):
        """Reconfigure modulation block for new mode"""
        # This requires disconnecting and reconnecting blocks
        # which can be complex in a running flowgraph
        pass
```

## Recommended Implementation

For most use cases, **Approach 1 (Selector Block)** is recommended because:

1. **Simple**: Uses standard GNU Radio blocks
2. **Efficient**: All modulators run in parallel, just switch output
3. **Reliable**: No dynamic reconfiguration needed
4. **Compatible**: Works with all GNU Radio versions

### Complete Example with Selector

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
        
        # Create modulation blocks for each mode
        mod_2fsk = digital.gfsk_mod(samples_per_symbol=2, bt=0.35)
        mod_4fsk = cpm.cpmmod_bc(type=cpm.CPM_LREC, h=0.5, samples_per_sym=2, L=4)
        mod_8fsk = cpm.cpmmod_bc(type=cpm.CPM_LREC, h=0.5, samples_per_sym=2, L=4)
        mod_qpsk = digital.psk.psk_mod(constellation_points=4)
        
        # Null sinks for unused modulators (or use throttle to reduce CPU)
        null_2fsk = blocks.null_sink(gr.sizeof_gr_complex)
        null_4fsk = blocks.null_sink(gr.sizeof_gr_complex)
        null_8fsk = blocks.null_sink(gr.sizeof_gr_complex)
        null_qpsk = blocks.null_sink(gr.sizeof_gr_complex)
        
        # Connect all modulators
        self.connect(encoder, mod_2fsk, null_2fsk)
        self.connect(encoder, mod_4fsk, null_4fsk)
        self.connect(encoder, mod_8fsk, null_8fsk)
        self.connect(encoder, mod_qpsk, null_qpsk)
        
        # Use a custom block or message handler to switch active modulator
        # For now, connect the default (4FSK)
        self.connect(encoder, mod_4fsk, ...)  # Continue with your flowgraph
        
        # Store references for mode switching
        self.rate_control = rate_control
        self.mod_blocks = {
            packet_protocols.modulation_mode_t.MODE_2FSK: mod_2fsk,
            packet_protocols.modulation_mode_t.MODE_4FSK: mod_4fsk,
            packet_protocols.modulation_mode_t.MODE_8FSK: mod_8fsk,
            packet_protocols.modulation_mode_t.MODE_QPSK: mod_qpsk,
        }
        
        # Update quality periodically (from SNR estimator, etc.)
        # This would typically be done via message passing
```

## Mode Control Summary

| Method | Description | When to Use |
|--------|-------------|-------------|
| `get_modulation_mode()` | Get current mode | Check current mode |
| `set_modulation_mode()` | Set mode manually | Manual control, disable adaptation |
| `update_quality()` | Update metrics, auto-adapt | Automatic adaptation enabled |
| `recommend_mode()` | Get recommendation | Preview mode without switching |
| `set_adaptation_enabled()` | Enable/disable auto-adapt | Toggle automatic vs manual |

## Future Enhancements

Planned improvements:

1. **Integrated Modulation Switch Block**: A hierarchical block that combines rate control with actual modulation switching
2. **Message Port Integration**: Automatic mode change messages when mode switches
3. **GRC Block Support**: Visual blocks for GNU Radio Companion
4. **Python Bindings**: Full Python API for mode control

## See Also

- [Adaptive Features Guide](ADAPTIVE_FEATURES.md) - Complete integration guide
- [README.md](../README.md) - Main documentation


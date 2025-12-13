# Making Your Flowgraph Speed Adaptive

This guide shows you how to convert your existing flowgraph (Vector Source -> Throttle -> AX.25 Encoder -> KISS TNC -> Null Sink) into a speed-adaptive system.

## Current Flowgraph

```
[Vector Source] -> [Throttle] -> [AX.25 Encoder] -> [KISS TNC] -> [Null Sink]
```

## Adaptive Flowgraph (Option 1: Using Adaptive Modulator Block)

The simplest approach uses the `adaptive_modulator` block which combines all modulation blocks and switching:

```
[Vector Source] -> [Throttle] -> [AX.25 Encoder] -> [Link Quality Monitor] -> [Adaptive Modulator] -> [KISS TNC] -> [Null Sink]
                                                                                    |
                                                                                    v
                                                                        [Adaptive Rate Control]
                                                                                    |
                                                                                    v
                                                                        [Quality Updates (SNR/BER)]
```

### Steps:

1. **Add Link Quality Monitor** after the AX.25 Encoder
2. **Add Adaptive Modulator** after the Link Quality Monitor (replaces need for individual modulators)
3. **Add Adaptive Rate Control** (already included in Adaptive Modulator, but you need access to it)
4. **Add quality monitoring** to feed SNR/BER metrics
5. **Connect KISS TNC** after the modulator (or replace with your actual sink)

### Python Code Example:

```python
from gnuradio import gr, blocks, digital
from gnuradio import packet_protocols
import threading
import time

class adaptive_flowgraph(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "Adaptive Packet Flowgraph")
        
        # Your existing blocks
        self.vector_source = blocks.vector_source_b([0x48, 0x65, 0x6C, 0x6C, 0x6F], True, 1, [])
        self.throttle = blocks.throttle(gr.sizeof_char * 1, 9600, True)
        self.ax25_encoder = packet_protocols.ax25_encoder('N0CALL', '0', 'N1CALL', '0', '', False, False)
        
        # NEW: Add Link Quality Monitor
        self.quality_monitor = packet_protocols.link_quality_monitor(
            alpha=0.1,
            update_period=1000
        )
        
        # NEW: Add Adaptive Modulator (includes rate control internally)
        self.adaptive_mod = packet_protocols.adaptive_modulator(
            initial_mode=packet_protocols.modulation_mode_t.MODE_4FSK,
            samples_per_symbol=2,
            enable_adaptation=True,
            hysteresis_db=2.0
        )
        
        # Get rate control from adaptive modulator
        self.rate_control = self.adaptive_mod.get_rate_control()
        
        # Your existing KISS TNC (or replace with actual sink)
        self.kiss_tnc = packet_protocols.kiss_tnc('/dev/ttyUSB0', 9600, False)
        self.null_sink = blocks.null_sink(gr.sizeof_gr_complex * 1)
        
        # Connect blocks
        self.connect(self.vector_source, self.throttle)
        self.connect(self.throttle, self.ax25_encoder)
        self.connect(self.ax25_encoder, self.quality_monitor)
        self.connect(self.quality_monitor, self.adaptive_mod)
        # Choose one: KISS TNC (for hardware) or null sink (for testing)
        # self.connect(self.adaptive_mod, self.kiss_tnc)
        self.connect(self.adaptive_mod, self.null_sink)
        
        # Setup quality updates
        self._setup_quality_updates()
    
    def _setup_quality_updates(self):
        """Setup periodic quality updates"""
        def update_quality():
            while True:
                time.sleep(1.0)  # Update every second
                
                # Get quality metrics (in real system, these come from SNR estimator, etc.)
                # For now, simulate with varying values
                import random
                snr_db = 10.0 + random.uniform(-5, 10)  # SNR between 5-20 dB
                ber = max(0.0, min(0.01, 0.001 * (20 - snr_db) / 10))
                quality_score = min(1.0, max(0.0, (snr_db - 5) / 20))
                
                # Update link quality monitor
                self.quality_monitor.update_snr(snr_db)
                self.quality_monitor.update_ber(ber)
                
                # Update adaptive rate control
                self.rate_control.update_quality(
                    snr_db, ber, quality_score
                )
                
                # Get current mode
                current_mode = self.rate_control.get_modulation_mode()
                current_rate = self.rate_control.get_data_rate()
                
                print(f"SNR: {snr_db:.1f} dB, BER: {ber:.4f}, Mode: {current_mode}, Rate: {current_rate} bps")
                
                # Update adaptive modulator mode (if needed)
                self.adaptive_mod.set_modulation_mode(current_mode)
        
        # Start quality update thread
        quality_thread = threading.Thread(target=update_quality, daemon=True)
        quality_thread.start()
```

## Adaptive Flowgraph (Option 2: Manual Block Setup)

For more control, add blocks individually:

```
[Vector Source] -> [Throttle] -> [AX.25 Encoder] -> [Link Quality Monitor] -> [Multiple Modulators] -> [Modulation Switch] -> [KISS TNC] -> [Null Sink]
                                                                                    |
                                                                                    v
                                                                        [Adaptive Rate Control]
                                                                                    |
                                                                                    v
                                                                        [Quality Updates (SNR/BER)]
```

### Python Code Example:

```python
from gnuradio import gr, blocks, digital
from gnuradio.analog import cpm
from gnuradio import packet_protocols
from gnuradio.packet_protocols import modulation_switch
import threading
import time

class adaptive_flowgraph_manual(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "Adaptive Packet Flowgraph (Manual)")
        
        # Your existing blocks
        self.vector_source = blocks.vector_source_b([0x48, 0x65, 0x6C, 0x6C, 0x6F], True, 1, [])
        self.throttle = blocks.throttle(gr.sizeof_char * 1, 9600, True)
        self.ax25_encoder = packet_protocols.ax25_encoder('N0CALL', '0', 'N1CALL', '0', '', False, False)
        
        # NEW: Link Quality Monitor
        self.quality_monitor = packet_protocols.link_quality_monitor(
            alpha=0.1,
            update_period=1000
        )
        
        # NEW: Adaptive Rate Control
        self.rate_control = packet_protocols.adaptive_rate_control(
            initial_mode=packet_protocols.modulation_mode_t.MODE_4FSK,
            enable_adaptation=True,
            hysteresis_db=2.0
        )
        
        # NEW: Create modulation blocks
        samples_per_symbol = 2
        self.mod_2fsk = digital.gfsk_mod(samples_per_symbol=samples_per_symbol, sensitivity=1.0, bt=0.35)
        self.mod_4fsk = cpm.cpmmod_bc(type=cpm.CPM_LREC, h=0.5, samples_per_sym=samples_per_symbol, L=4, beta=0.3)
        self.mod_8fsk = cpm.cpmmod_bc(type=cpm.CPM_LREC, h=0.5, samples_per_sym=samples_per_symbol, L=4, beta=0.3)
        self.mod_qpsk = digital.psk.psk_mod(constellation_points=4, mod_code=digital.GRAY_CODE, differential=False)
        
        # Mode to index mapping
        self.mode_to_index = {
            packet_protocols.modulation_mode_t.MODE_2FSK: 0,
            packet_protocols.modulation_mode_t.MODE_4FSK: 1,
            packet_protocols.modulation_mode_t.MODE_8FSK: 2,
            packet_protocols.modulation_mode_t.MODE_QPSK: 3,
        }
        
        # NEW: Modulation Switch
        self.mod_switch = modulation_switch(
            self.rate_control,
            self.mode_to_index,
            num_inputs=4
        )
        
        # Your existing KISS TNC (or replace with actual sink)
        self.kiss_tnc = packet_protocols.kiss_tnc('/dev/ttyUSB0', 9600, False)
        self.null_sink = blocks.null_sink(gr.sizeof_gr_complex * 1)
        
        # Connect blocks
        self.connect(self.vector_source, self.throttle)
        self.connect(self.throttle, self.ax25_encoder)
        self.connect(self.ax25_encoder, self.quality_monitor)
        
        # Connect quality monitor to all modulators
        self.connect((self.quality_monitor, 0), (self.mod_2fsk, 0))
        self.connect((self.quality_monitor, 0), (self.mod_4fsk, 0))
        self.connect((self.quality_monitor, 0), (self.mod_8fsk, 0))
        self.connect((self.quality_monitor, 0), (self.mod_qpsk, 0))
        
        # Connect modulators to modulation switch
        self.connect((self.mod_2fsk, 0), (self.mod_switch, 0))
        self.connect((self.mod_4fsk, 0), (self.mod_switch, 1))
        self.connect((self.mod_8fsk, 0), (self.mod_switch, 2))
        self.connect((self.mod_qpsk, 0), (self.mod_switch, 3))
        
        # Connect modulation switch to output
        # Choose one: KISS TNC (for hardware) or null sink (for testing)
        # self.connect((self.mod_switch, 0), self.kiss_tnc)
        self.connect((self.mod_switch, 0), self.null_sink)
        
        # Setup quality updates
        self._setup_quality_updates()
    
    def _setup_quality_updates(self):
        """Setup periodic quality updates"""
        def update_quality():
            while True:
                time.sleep(1.0)  # Update every second
                
                # Get quality metrics (in real system, these come from SNR estimator, etc.)
                import random
                snr_db = 10.0 + random.uniform(-5, 10)  # SNR between 5-20 dB
                ber = max(0.0, min(0.01, 0.001 * (20 - snr_db) / 10))
                quality_score = min(1.0, max(0.0, (snr_db - 5) / 20))
                
                # Update link quality monitor
                self.quality_monitor.update_snr(snr_db)
                self.quality_monitor.update_ber(ber)
                
                # Update adaptive rate control
                self.rate_control.update_quality(
                    snr_db, ber, quality_score
                )
                
                # Get current mode
                current_mode = self.rate_control.get_modulation_mode()
                current_rate = self.rate_control.get_data_rate()
                
                print(f"SNR: {snr_db:.1f} dB, BER: {ber:.4f}, Mode: {current_mode}, Rate: {current_rate} bps")
                
                # Send mode update message to modulation switch
                from gnuradio import pmt
                self.mod_switch.message_port_pub(
                    pmt.intern("mode_update"),
                    pmt.PMT_NIL
                )
        
        # Start quality update thread
        quality_thread = threading.Thread(target=update_quality, daemon=True)
        quality_thread.start()
```

## Key Points

1. **Link Quality Monitor** - Tracks SNR, BER, and frame error rate. Must be updated periodically with quality metrics.

2. **Adaptive Rate Control** - Automatically selects the best modulation mode based on quality metrics. Call `update_quality(snr_db, ber, quality_score)` regularly.

3. **Modulation Blocks** - Create modulators for each mode you want to support (2FSK, 4FSK, 8FSK, QPSK, etc.).

4. **Modulation Switch** - Switches between modulator outputs based on the current mode selected by rate control.

5. **Quality Updates** - You need to feed SNR/BER measurements to the system. In a real system, these come from:
   - SNR estimator (e.g., `digital.probe_mpsk_snr_est_c`)
   - BER measurement from frame validation
   - Frame error rate from successful/failed frame counts

## Real-World Quality Measurement

In a real system, you'd measure quality from your receive path:

```python
from gnuradio import digital
from gnuradio import pmt

# Create SNR estimator in your receive path
snr_estimator = digital.probe_mpsk_snr_est_c(
    type=digital.SNR_EST_SIMPLE,
    msg_nsamples=1000,
    alpha=0.001
)

# Connect to receive signal
# self.connect(receive_signal, snr_estimator)

# Handle SNR messages
def handle_snr(msg):
    snr_db = pmt.to_python(msg)
    quality_monitor.update_snr(snr_db)

snr_estimator.message_port_register_out(pmt.intern("snr"))
# Setup message handler to call handle_snr when SNR updates
```

## GRC (GNU Radio Companion) Setup

If using GRC, add these blocks:

1. **Packet Protocols > Link Quality Monitor** - After AX.25 Encoder
2. **Packet Protocols > Adaptive Modulator** - After Link Quality Monitor
   - OR manually add multiple modulation blocks and a selector
3. **Packet Protocols > Adaptive Rate Control** - For manual setup
4. **Digital > GFSK Mod, CPM Mod, PSK Mod, QAM Mod** - For manual setup
5. **Blocks > Selector** - For manual setup

## Notes

- The KISS TNC expects byte input, but modulators output complex samples. You may need to:
  - Remove KISS TNC if doing software-defined radio
  - Add a complex-to-byte converter if your hardware needs bytes
  - Use a different sink (file, USRP, etc.) for complex samples

- Quality updates drive adaptation - without regular SNR/BER updates, the system won't adapt.

- The system adapts based on **received** signal quality, so you need a receive path to measure quality.

## See Also

- [Adaptive Features Guide](ADAPTIVE_FEATURES.md) - Detailed feature documentation
- [Complete Integration Guide](COMPLETE_INTEGRATION_GUIDE.md) - Full integration details
- [Example Code](../examples/adaptive_modulation_example.py) - Working example


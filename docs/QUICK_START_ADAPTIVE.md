# Quick Start: Adaptive Modulation

This is a quick reference for setting up adaptive modulation. For detailed information, see [COMPLETE_INTEGRATION_GUIDE.md](COMPLETE_INTEGRATION_GUIDE.md).

## Minimal Example

```python
from gnuradio import gr, blocks, digital
from gnuradio.analog import cpm
from gnuradio import packet_protocols
from gnuradio.packet_protocols import modulation_switch

class my_flowgraph(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        
        # 1. Create blocks
        encoder = packet_protocols.ax25_encoder('N0CALL', '0', 'N1CALL', '0')
        quality_monitor = packet_protocols.link_quality_monitor()
        rate_control = packet_protocols.adaptive_rate_control(
            initial_mode=packet_protocols.modulation_mode_t.MODE_4FSK
        )
        
        # 2. Create modulators
        mod_2fsk = digital.gfsk_mod(samples_per_symbol=2, bt=0.35)
        mod_4fsk = cpm.cpmmod_bc(type=cpm.CPM_LREC, h=0.5, samples_per_sym=2, L=4)
        mod_qpsk = digital.psk.psk_mod(constellation_points=4)
        
        # 3. Create switch
        mode_map = {
            packet_protocols.modulation_mode_t.MODE_2FSK: 0,
            packet_protocols.modulation_mode_t.MODE_4FSK: 1,
            packet_protocols.modulation_mode_t.MODE_QPSK: 2,
        }
        mod_switch = modulation_switch(rate_control, mode_map, num_inputs=3)
        
        # 4. Connect
        self.connect(source, encoder, quality_monitor)
        self.connect((quality_monitor, 0), (mod_2fsk, 0))
        self.connect((quality_monitor, 0), (mod_4fsk, 0))
        self.connect((quality_monitor, 0), (mod_qpsk, 0))
        self.connect((mod_2fsk, 0), (mod_switch, 0))
        self.connect((mod_4fsk, 0), (mod_switch, 1))
        self.connect((mod_qpsk, 0), (mod_switch, 2))
        self.connect((mod_switch, 0), sink)
        
        # 5. Update quality periodically
        import threading
        def update():
            while True:
                time.sleep(1.0)
                snr = get_snr()  # Your function
                ber = get_ber()  # Your function
                quality_monitor.update_snr(snr)
                quality_monitor.update_ber(ber)
                quality_score = quality_monitor.get_quality_score()
                rate_control.update_quality(snr, ber, quality_score)
                mod_switch.message_port_pub(pmt.intern("mode_update"), pmt.PMT_NIL)
        threading.Thread(target=update, daemon=True).start()
```

## Key Components

1. **Link Quality Monitor** - Tracks SNR, BER, FER
2. **Adaptive Rate Control** - Selects optimal mode
3. **Modulation Blocks** - Actual modulators (FSK, PSK, QAM)
4. **Adaptive Modulator** - Python hierarchical block that automatically switches between modulators (recommended for GRC)
5. **Modulation Switch** - Helper block for programmatic use (uses GNU Radio's selector in GRC)
6. **Modulation Negotiation** - Coordinates mode changes between stations (optional, for inter-station communication)

## Connection Pattern

```
[Data] -> [Encoder] -> [Quality Monitor] -> [All Modulators] -> [Switch] -> [Output]
                                                      |
                                                      v
                                          [Rate Control] (controls switch)
```

## Adding Inter-Station Negotiation (Optional)

For coordinated mode changes between stations:

```python
# Create negotiator
negotiator = packet_protocols.modulation_negotiation(
    station_id="N0CALL",
    supported_modes=[
        packet_protocols.modulation_mode_t.MODE_2FSK,
        packet_protocols.modulation_mode_t.MODE_4FSK,
        packet_protocols.modulation_mode_t.MODE_8FSK
    ]
)

# If using KISS TNC, connect message ports
# tnc.msg_connect(tnc, "negotiation_out", negotiator, "negotiation_in")

# Set callback for sending frames (see ADAPTIVE_FEATURES.md for details)
negotiator.set_kiss_frame_sender(send_kiss_frame_callback)

# Enable automatic negotiation
negotiator.set_auto_negotiation_enabled(True, rate_control)
```

## See Also

- [Complete Integration Guide](COMPLETE_INTEGRATION_GUIDE.md) - Full details
- [Adaptive Features Guide](ADAPTIVE_FEATURES.md) - Feature overview including negotiation
- [Adaptive System Architecture](ADAPTIVE_SYSTEM_ARCHITECTURE.md) - Detailed architecture
- [Example Code](../examples/adaptive_modulation_example.py) - Working example


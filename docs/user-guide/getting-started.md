# Getting Started with gr-packet-protocols

This guide will help you get started with using the gr-packet-protocols module in GNU Radio.

## Overview

The gr-packet-protocols module provides GNU Radio blocks for implementing various packet radio protocols:

- **AX.25**: Traditional amateur radio packet protocol
- **FX.25**: Forward Error Correction for AX.25
- **IL2P**: Improved Layer 2 Protocol
- **KISS TNC**: Terminal Node Controller interface

## Basic Usage

### 1. Starting GNU Radio Companion

```bash
gnuradio-companion
```

### 2. Adding Protocol Blocks

1. **Open GNU Radio Companion**
2. **Navigate to the blocks panel** (usually on the right)
3. **Search for "packet_protocols"** in the search box
4. **Drag and drop** the desired protocol blocks into your flowgraph

### 3. Available Blocks

#### AX.25 Protocol Blocks
- **AX.25 Encoder**: Encodes data into AX.25 frames
- **AX.25 Decoder**: Decodes AX.25 frames into data
- **KISS TNC**: Terminal Node Controller interface

#### FX.25 Protocol Blocks
- **FX.25 Encoder**: Adds Forward Error Correction to AX.25 frames
- **FX.25 Decoder**: Decodes FEC-protected AX.25 frames

#### IL2P Protocol Blocks
- **IL2P Encoder**: Encodes data into IL2P frames
- **IL2P Decoder**: Decodes IL2P frames into data

## Quick Examples

### Example 1: Basic AX.25 Encoding

```python
#!/usr/bin/env python3

import numpy as np
from gnuradio import gr, blocks, packet_protocols

class ax25_example(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "AX.25 Example")
        
        # Create blocks
        self.ax25_encoder = packet_protocols.ax25_encoder()
        self.ax25_decoder = packet_protocols.ax25_decoder()
        
        # Connect blocks
        self.connect(self.ax25_encoder, self.ax25_decoder)
        
        # Set parameters
        self.ax25_encoder.set_dest_callsign("APRS")
        self.ax25_encoder.set_src_callsign("N0CALL")
        self.ax25_encoder.set_ssid(0)

if __name__ == '__main__':
    tb = ax25_example()
    tb.start()
    tb.wait()
```

### Example 2: FX.25 with Forward Error Correction

```python
#!/usr/bin/env python3

from gnuradio import gr, packet_protocols

class fx25_example(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "FX.25 Example")
        
        # Create blocks
        self.fx25_encoder = packet_protocols.fx25_encoder()
        self.fx25_decoder = packet_protocols.fx25_decoder()
        
        # Connect blocks
        self.connect(self.fx25_encoder, self.fx25_decoder)
        
        # Set FEC parameters
        self.fx25_encoder.set_fec_type(0)  # Reed-Solomon (255,223)
        self.fx25_encoder.set_interleaving(1)

if __name__ == '__main__':
    tb = fx25_example()
    tb.start()
    tb.wait()
```

### Example 3: IL2P Modern Protocol

```python
#!/usr/bin/env python3

from gnuradio import gr, packet_protocols

class il2p_example(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "IL2P Example")
        
        # Create blocks
        self.il2p_encoder = packet_protocols.il2p_encoder()
        self.il2p_decoder = packet_protocols.il2p_decoder()
        
        # Connect blocks
        self.connect(self.il2p_encoder, self.il2p_decoder)
        
        # Set IL2P parameters
        self.il2p_encoder.set_header_type(0)  # Data frame
        self.il2p_encoder.set_priority(0)     # Normal priority

if __name__ == '__main__':
    tb = il2p_example()
    tb.start()
    tb.wait()
```

## Configuration Parameters

### AX.25 Encoder Parameters
- **Dest Callsign**: Destination station callsign (e.g., "APRS")
- **Src Callsign**: Source station callsign (e.g., "N0CALL")
- **SSID**: Secondary Station Identifier (0-15)
- **Control Field**: Frame control field (I, S, or U frame)
- **PID**: Protocol Identifier (0xF0 for no layer 3)

### FX.25 Encoder Parameters
- **FEC Type**: Forward Error Correction type (0=Reed-Solomon)
- **Interleaving**: Interleaving depth (1-8)
- **Correlation Tag**: Frame correlation identifier

### IL2P Encoder Parameters
- **Header Type**: Frame header type (0=Data, 1=Control)
- **Priority**: Frame priority level (0-7)
- **Scrambling**: Data scrambling enabled/disabled

## Common Use Cases

### 1. APRS Position Reporting
```python
# Configure for APRS position report
ax25_encoder.set_dest_callsign("APRS")
ax25_encoder.set_src_callsign("N0CALL-1")
ax25_encoder.set_ssid(1)
ax25_encoder.set_control_field(0x03)  # UI frame
ax25_encoder.set_pid(0xF0)            # No layer 3
```

### 2. Data Transmission with FEC
```python
# Configure for reliable data transmission
fx25_encoder.set_fec_type(0)          # Reed-Solomon
fx25_encoder.set_interleaving(2)     # Moderate interleaving
fx25_encoder.set_correlation_tag(0x1234)
```

### 3. Modern IL2P Communication
```python
# Configure for modern packet radio
il2p_encoder.set_header_type(0)       # Data frame
il2p_encoder.set_priority(0)          # Normal priority
il2p_encoder.set_scrambling(True)    # Enable scrambling
```

## Troubleshooting

### Common Issues

1. **Block Not Found**: Ensure the module is properly installed
2. **Connection Errors**: Check data types and sample rates
3. **Parameter Errors**: Verify parameter ranges and formats

### Debug Tips

1. **Enable Debug Output**: Set verbosity levels in block parameters
2. **Check Data Flow**: Use probe blocks to monitor data
3. **Verify Configuration**: Double-check all parameter settings

## Next Steps

- **Explore Examples**: See [Examples](../examples/) for more complex scenarios
- **Read API Documentation**: Check [API Reference](../api/) for detailed information
- **Learn Protocol Details**: Study [Protocol Specifications](../protocols/) for in-depth understanding

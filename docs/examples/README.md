# Examples Documentation

This directory contains comprehensive examples and tutorials for using the gr-packet-protocols module.

## Example Categories

### 1. Basic Examples
- **Simple Protocol Usage**: Basic encoding/decoding examples
- **Parameter Configuration**: How to configure protocol parameters
- **Data Flow**: Understanding input/output data flow

### 2. Advanced Examples
- **Complex Scenarios**: Multi-protocol combinations
- **Performance Optimization**: Efficient processing techniques
- **Error Handling**: Robust error handling patterns

### 3. Real-World Applications
- **APRS Systems**: Automatic Position Reporting System examples
- **Data Networks**: Packet radio data networks
- **Emergency Communications**: Emergency communication systems

## Example Structure

```
examples/
├── basic/           # Basic usage examples
├── advanced/        # Advanced scenarios
├── applications/    # Real-world applications
├── grc/            # GNU Radio Companion flowgraphs
└── python/         # Python script examples
```

## Quick Start Examples

### 1. Basic AX.25 Encoding
```python
#!/usr/bin/env python3
from gnuradio import gr, packet_protocols

# Create a simple AX.25 encoder
encoder = packet_protocols.ax25_encoder()
encoder.set_dest_callsign("APRS")
encoder.set_src_callsign("N0CALL")
encoder.set_ssid(0)

# Encode some data
data = "Hello World"
encoded = encoder.encode(data)
print(f"Encoded: {encoded.hex()}")
```

### 2. FX.25 with Forward Error Correction
```python
#!/usr/bin/env python3
from gnuradio import gr, packet_protocols

# Create FX.25 encoder with FEC
fec_encoder = packet_protocols.fx25_encoder()
fec_encoder.set_fec_type(0)  # Reed-Solomon
fec_encoder.set_interleaving(2)

# Encode with error correction
data = "Important Message"
fec_encoded = fec_encoder.encode(data)
print(f"FEC Encoded: {fec_encoded.hex()}")
```

### 3. IL2P Modern Protocol
```python
#!/usr/bin/env python3
from gnuradio import gr, packet_protocols

# Create IL2P encoder
il2p_encoder = packet_protocols.il2p_encoder()
il2p_encoder.set_header_type(0)  # Data frame
il2p_encoder.set_priority(0)      # Normal priority
il2p_encoder.set_scrambling(True)

# Encode modern packet
data = "Modern Packet Data"
il2p_encoded = il2p_encoder.encode(data)
print(f"IL2P Encoded: {il2p_encoded.hex()}")
```

## GNU Radio Companion Examples

### 1. AX.25 KISS Example
**File**: `examples/grc/ax25_kiss_example.grc`

**Description**: Complete AX.25 KISS TNC implementation
**Features**:
- KISS TNC interface
- AX.25 encoding/decoding
- Data source and sink
- Real-time processing

### 2. FX.25 FEC Example
**File**: `examples/grc/fx25_fec_example.grc`

**Description**: FX.25 Forward Error Correction example
**Features**:
- FEC encoding/decoding
- Error injection simulation
- Performance monitoring
- Statistical analysis

### 3. IL2P Example
**File**: `examples/grc/il2p_example.grc`

**Description**: IL2P modern protocol example
**Features**:
- IL2P encoding/decoding
- Header processing
- Payload scrambling
- Reed-Solomon FEC

## Python Script Examples

### 1. APRS Position Reporting
**File**: `examples/python/aprs_position.py`

```python
#!/usr/bin/env python3
import time
from gnuradio import gr, packet_protocols

class APRSPositionReporter(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "APRS Position Reporter")
        
        # Create AX.25 encoder for APRS
        self.encoder = packet_protocols.ax25_encoder()
        self.encoder.set_dest_callsign("APRS")
        self.encoder.set_src_callsign("N0CALL-1")
        self.encoder.set_ssid(1)
        self.encoder.set_control_field(0x03)  # UI frame
        self.encoder.set_pid(0xF0)            # No layer 3
        
    def send_position(self, latitude, longitude, comment=""):
        # Format APRS position report
        position_data = f"!{latitude}/{longitude}{comment}"
        
        # Encode and send
        encoded = self.encoder.encode(position_data)
        print(f"Sent position: {position_data}")
        return encoded

# Usage
if __name__ == '__main__':
    reporter = APRSPositionReporter()
    
    # Send position report
    reporter.send_position("4903.50N", "07201.75W", "-Test")
```

### 2. Data Transmission with Acknowledgment
**File**: `examples/python/data_transmission.py`

```python
#!/usr/bin/env python3
from gnuradio import gr, packet_protocols

class DataTransmission(gr.top_block):
    def __init__(self, dest_callsign, src_callsign):
        gr.top_block.__init__(self, "Data Transmission")
        
        # Create encoder and decoder
        self.encoder = packet_protocols.ax25_encoder()
        self.decoder = packet_protocols.ax25_decoder()
        
        # Configure addressing
        self.encoder.set_dest_callsign(dest_callsign)
        self.encoder.set_src_callsign(src_callsign)
        
        # Configure for I-frames (data transmission)
        self.encoder.set_control_field(0x00)  # I-frame
        self.encoder.set_pid(0xCC)            # AX.25 text
        
    def send_data(self, data):
        # Send data with sequence numbers
        self.encoder.set_ns(self.get_next_sequence())
        encoded = self.encoder.encode(data)
        print(f"Sent data: {data}")
        return encoded
        
    def get_next_sequence(self):
        # Implement sequence number management
        return (self.sequence_number + 1) % 8
```

### 3. Multi-Protocol Gateway
**File**: `examples/python/protocol_gateway.py`

```python
#!/usr/bin/env python3
from gnuradio import gr, packet_protocols

class ProtocolGateway(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "Protocol Gateway")
        
        # Create protocol encoders/decoders
        self.ax25_encoder = packet_protocols.ax25_encoder()
        self.fx25_encoder = packet_protocols.fx25_encoder()
        self.il2p_encoder = packet_protocols.il2p_encoder()
        
        # Configure protocols
        self.setup_ax25()
        self.setup_fx25()
        self.setup_il2p()
        
    def setup_ax25(self):
        self.ax25_encoder.set_dest_callsign("GATEWAY")
        self.ax25_encoder.set_src_callsign("N0CALL")
        
    def setup_fx25(self):
        self.fx25_encoder.set_fec_type(0)  # Reed-Solomon
        self.fx25_encoder.set_interleaving(2)
        
    def setup_il2p(self):
        self.il2p_encoder.set_header_type(0)  # Data frame
        self.il2p_encoder.set_priority(0)      # Normal priority
        
    def convert_ax25_to_fx25(self, ax25_data):
        # Convert AX.25 to FX.25 with FEC
        return self.fx25_encoder.encode(ax25_data)
        
    def convert_fx25_to_il2p(self, fx25_data):
        # Convert FX.25 to IL2P
        return self.il2p_encoder.encode(fx25_data)
```

## Advanced Examples

### 1. Performance Optimization
**File**: `examples/advanced/performance_optimization.py`

```python
#!/usr/bin/env python3
import time
import numpy as np
from gnuradio import gr, packet_protocols

class OptimizedProcessor(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "Optimized Processor")
        
        # Use efficient data types
        self.encoder = packet_protocols.ax25_encoder()
        self.decoder = packet_protocols.ax25_decoder()
        
        # Pre-allocate buffers
        self.buffer_size = 1024
        self.input_buffer = np.zeros(self.buffer_size, dtype=np.uint8)
        self.output_buffer = np.zeros(self.buffer_size, dtype=np.uint8)
        
    def process_batch(self, data_batch):
        # Process multiple frames efficiently
        results = []
        for data in data_batch:
            encoded = self.encoder.encode(data)
            decoded = self.decoder.decode(encoded)
            results.append(decoded)
        return results
```

### 2. Error Handling and Recovery
**File**: `examples/advanced/error_handling.py`

```python
#!/usr/bin/env python3
from gnuradio import gr, packet_protocols
import logging

class RobustProcessor(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self, "Robust Processor")
        
        # Setup logging
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
        
        # Create encoders/decoders with error handling
        self.encoder = packet_protocols.ax25_encoder()
        self.decoder = packet_protocols.ax25_decoder()
        
        # Configure error handling
        self.max_retries = 3
        self.timeout_ms = 1000
        
    def safe_encode(self, data):
        """Safely encode data with error handling."""
        try:
            if not data or len(data) == 0:
                raise ValueError("Empty data")
                
            if len(data) > 256:
                raise ValueError("Data too long")
                
            encoded = self.encoder.encode(data)
            self.logger.info(f"Successfully encoded {len(data)} bytes")
            return encoded
            
        except Exception as e:
            self.logger.error(f"Encoding failed: {e}")
            return None
            
    def safe_decode(self, encoded_data):
        """Safely decode data with error handling."""
        try:
            if not encoded_data or len(encoded_data) == 0:
                raise ValueError("Empty encoded data")
                
            decoded = self.decoder.decode(encoded_data)
            self.logger.info(f"Successfully decoded {len(decoded)} bytes")
            return decoded
            
        except Exception as e:
            self.logger.error(f"Decoding failed: {e}")
            return None
```

## Real-World Applications

### 1. Emergency Communication System
**File**: `examples/applications/emergency_comm.py`

```python
#!/usr/bin/env python3
from gnuradio import gr, packet_protocols
import json
import time

class EmergencyCommSystem(gr.top_block):
    def __init__(self, callsign):
        gr.top_block.__init__(self, "Emergency Comm System")
        
        self.callsign = callsign
        self.encoder = packet_protocols.ax25_encoder()
        self.decoder = packet_protocols.ax25_decoder()
        
        # Configure for emergency communications
        self.encoder.set_dest_callsign("EMERGENCY")
        self.encoder.set_src_callsign(callsign)
        self.encoder.set_control_field(0x03)  # UI frame
        self.encoder.set_pid(0xF0)            # No layer 3
        
    def send_emergency_message(self, message_type, data):
        """Send emergency message."""
        emergency_data = {
            "type": message_type,
            "timestamp": time.time(),
            "callsign": self.callsign,
            "data": data
        }
        
        # Convert to JSON and encode
        json_data = json.dumps(emergency_data)
        encoded = self.encoder.encode(json_data)
        
        print(f"Emergency message sent: {message_type}")
        return encoded
        
    def send_status_update(self, status):
        """Send status update."""
        return self.send_emergency_message("STATUS", status)
        
    def send_location_update(self, latitude, longitude):
        """Send location update."""
        location = f"{latitude},{longitude}"
        return self.send_emergency_message("LOCATION", location)
```

### 2. Packet Radio Network Node
**File**: `examples/applications/network_node.py`

```python
#!/usr/bin/env python3
from gnuradio import gr, packet_protocols
import threading
import queue

class NetworkNode(gr.top_block):
    def __init__(self, node_id, neighbors):
        gr.top_block.__init__(self, f"Network Node {node_id}")
        
        self.node_id = node_id
        self.neighbors = neighbors
        self.message_queue = queue.Queue()
        
        # Create protocol handlers
        self.ax25_encoder = packet_protocols.ax25_encoder()
        self.ax25_decoder = packet_protocols.ax25_decoder()
        self.fx25_encoder = packet_protocols.fx25_encoder()
        
        # Configure for network operation
        self.setup_network_protocols()
        
        # Start message processing thread
        self.processing_thread = threading.Thread(target=self.process_messages)
        self.processing_thread.daemon = True
        self.processing_thread.start()
        
    def setup_network_protocols(self):
        """Setup network protocol configuration."""
        # Configure AX.25 for network addressing
        self.ax25_encoder.set_dest_callsign("NETWORK")
        self.ax25_encoder.set_src_callsign(f"NODE{self.node_id}")
        
        # Configure FX.25 for reliable transmission
        self.fx25_encoder.set_fec_type(0)  # Reed-Solomon
        self.fx25_encoder.set_interleaving(2)
        
    def send_message(self, dest_node, message):
        """Send message to destination node."""
        # Create network packet
        packet = {
            "dest": dest_node,
            "src": self.node_id,
            "message": message,
            "timestamp": time.time()
        }
        
        # Encode with FEC for reliability
        ax25_encoded = self.ax25_encoder.encode(str(packet))
        fx25_encoded = self.fx25_encoder.encode(ax25_encoded)
        
        print(f"Message sent to node {dest_node}: {message}")
        return fx25_encoded
        
    def process_messages(self):
        """Process incoming messages."""
        while True:
            try:
                # Process messages from queue
                if not self.message_queue.empty():
                    message = self.message_queue.get()
                    self.handle_message(message)
                    
            except Exception as e:
                print(f"Error processing message: {e}")
                
    def handle_message(self, message):
        """Handle received message."""
        print(f"Node {self.node_id} received: {message}")
        # Implement message handling logic
```

## Running Examples

### 1. Python Examples
```bash
# Make examples executable
chmod +x examples/python/*.py

# Run basic example
python3 examples/python/aprs_position.py

# Run advanced example
python3 examples/advanced/performance_optimization.py
```

### 2. GNU Radio Companion Examples
```bash
# Open GRC examples
gnuradio-companion examples/grc/ax25_kiss_example.grc
gnuradio-companion examples/grc/fx25_fec_example.grc
gnuradio-companion examples/grc/il2p_example.grc
```

### 3. Application Examples
```bash
# Run emergency communication system
python3 examples/applications/emergency_comm.py

# Run network node
python3 examples/applications/network_node.py
```

## Customization

### Modifying Examples
1. **Copy example files** to your working directory
2. **Modify parameters** to match your requirements
3. **Add your own logic** for specific use cases
4. **Test thoroughly** before deployment

### Creating New Examples
1. **Follow the structure** of existing examples
2. **Include documentation** and comments
3. **Test with various inputs** to ensure robustness
4. **Submit for inclusion** in the examples collection

## Troubleshooting

### Common Issues
1. **Import errors**: Ensure gr-packet-protocols is installed
2. **Parameter errors**: Check parameter ranges and formats
3. **Performance issues**: Optimize for your specific use case
4. **Compatibility**: Ensure GNU Radio version compatibility

### Getting Help
1. **Check documentation**: Review protocol and API documentation
2. **Run tests**: Use the provided test suites
3. **Community support**: Join the GNU Radio community
4. **Issue reporting**: Report bugs and request features

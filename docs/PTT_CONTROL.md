# PTT Control in GNU Radio Packet Radio

This document provides comprehensive guidance on implementing Push To Talk (PTT) control in GNU Radio packet radio applications.

## Overview

PTT control is essential for half-duplex packet radio operation. The transmitter must be keyed (PTT enabled) before transmission and unkeyed after transmission completes. This document covers multiple methods for implementing PTT control depending on your hardware configuration.

## Table of Contents

1. [Built-in KISS TNC PTT Control](#built-in-kiss-tnc-ptt-control)
2. [SDR Hardware Blocks](#sdr-hardware-blocks)
3. [Custom GPIO Control](#custom-gpio-control)
4. [Message-Based Control](#message-based-control)
5. [Timing Considerations](#timing-considerations)
6. [Hardware-Specific Examples](#hardware-specific-examples)

## Built-in KISS TNC PTT Control

The `gr-packet-protocols` KISS TNC block includes built-in PTT control via serial port DTR/RTS lines. This is the simplest method for traditional hardware TNCs.

### When to Use

- Connecting to hardware TNCs via serial port
- Simple setup with minimal configuration
- Standard KISS protocol implementations

### Implementation

See the main README.md for complete examples. Basic usage:

```python
tnc = packet_protocols.kiss_tnc("/dev/ttyUSB0", 9600)
tnc.set_ptt_enabled(True)
tnc.set_ptt_use_dtr(False)  # Use RTS (default)
```

## SDR Hardware Blocks

For Software Defined Radio (SDR) transmitters, PTT control is handled through hardware-specific blocks.

### gr-osmosdr (Osmocom SDR)

Osmocom SDR supports various devices including HackRF, BladeRF, RTL-SDR, and others.

#### Basic Usage

```python
from gnuradio import osmosdr

# Create osmocom sink
sdr = osmosdr.sink(args="numchan=1")

# For devices with GPIO support (e.g., HackRF)
# GPIO pin 0 is often used for PTT
sdr.set_gpio(0x01)  # Set GPIO pin 0
```

#### Device-Specific GPIO

Different SDR devices use different GPIO pin assignments:

- **HackRF**: GPIO pin 0 (bit 0) for PTT
- **BladeRF**: GPIO pins vary by model
- **RTL-SDR**: Typically no GPIO, requires external PTT control

#### Complete Example

```python
from gnuradio import gr, blocks, packet_protocols, osmosdr
from gnuradio.filter import firdes

class sdr_packet_tx(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        
        # Packet encoder
        encoder = packet_protocols.ax25_encoder(
            dest_callsign="N0CALL",
            dest_ssid="0",
            src_callsign="N1CALL",
            src_ssid="0"
        )
        
        # SDR sink (HackRF example)
        sdr = osmosdr.sink(
            args="numchan=1,hackrf=0"
        )
        sdr.set_sample_rate(9600)
        sdr.set_center_freq(144390000)  # 2m band
        sdr.set_gain(20)
        
        # Configure GPIO for PTT (HackRF specific)
        sdr.set_gpio(0x01)  # GPIO pin 0
        
        # Connect blocks
        # Note: PTT is typically controlled automatically by the driver
        # based on data presence, but GPIO can be used for external control
        self.connect(encoder, sdr)
```

### gr-iio (PlutoSDR)

PlutoSDR uses the IIO (Industrial I/O) subsystem for GPIO control. The PlutoSDR has several GPIO pins accessible through the IIO interface.

#### Prerequisites

The PlutoSDR PTT control block requires the `pylibiio` Python package:

```bash
pip install pylibiio
```

On some systems (e.g., newer Debian/Ubuntu), you may need to use the `--break-system-packages` flag:

```bash
pip install --break-system-packages pylibiio
```

**Note**: The `pylibiio` package provides Python bindings for the libiio library, which is required to control GPIO pins on PlutoSDR devices.

#### Basic Usage

```python
from gnuradio import iio
import iio as libiio

# Create PlutoSDR sink
pluto_sink = iio.pluto_sink(
    uri="ip:pluto.local",  # or "usb:1.2.5" for USB connection
    buffer_size=32768,
    cyclic=False
)

# Set frequency and sample rate
pluto_sink.set_params(
    144390000,  # Center frequency (Hz)
    9600000,    # Sample rate (Hz)
    20000000,   # Bandwidth (Hz)
    0,          # RF port select (0 = A, 1 = B)
    -89.75,     # Attenuation (dB)
    True        # Filter auto
)

# GPIO control via IIO context
ctx = libiio.Context("ip:pluto.local")
gpio = ctx.find_device("gpio")
gpio_attr = gpio.find_channel("out", False)  # False = output channel
gpio_attr.attrs[0].value = "1"  # Set GPIO pin (PTT on)
```

#### PlutoSDR GPIO Pins

PlutoSDR has GPIO pins accessible through the IIO subsystem:
- **GPIO 0-7**: Available on the expansion connector
- **GPIO control**: Via IIO device "gpio"

#### Complete Example with PTT Control

```python
from gnuradio import gr, blocks, packet_protocols, iio
import iio as libiio
import threading
import time

class pluto_ptt_control:
    def __init__(self, pluto_uri="ip:pluto.local", gpio_pin=0):
        self.ctx = libiio.Context(pluto_uri)
        self.gpio = self.ctx.find_device("gpio")
        self.gpio_attr = self.gpio.find_channel("out", False)
        self.gpio_pin = gpio_pin
        self.ptt_state = False
        
    def set_ptt(self, state):
        """Set PTT state (True = TX, False = RX)"""
        if state != self.ptt_state:
            # GPIO value is a bitmask
            current = int(self.gpio_attr.attrs[0].value) if self.gpio_attr.attrs[0].value else 0
            if state:
                # Set bit for PTT (TX mode)
                new_value = current | (1 << self.gpio_pin)
            else:
                # Clear bit for PTT (RX mode)
                new_value = current & ~(1 << self.gpio_pin)
            
            self.gpio_attr.attrs[0].value = str(new_value)
            self.ptt_state = state
            
    def cleanup(self):
        self.set_ptt(False)
        self.ctx = None

class pluto_packet_tx(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        
        # Packet encoder
        encoder = packet_protocols.ax25_encoder(
            dest_callsign="N0CALL",
            dest_ssid="0",
            src_callsign="N1CALL",
            src_ssid="0"
        )
        
        # PlutoSDR sink
        pluto_sink = iio.pluto_sink(
            uri="ip:pluto.local",
            buffer_size=32768,
            cyclic=False
        )
        pluto_sink.set_params(
            144390000,  # 2m band
            9600000,
            20000000,
            0,
            -89.75,
            True
        )
        
        # PTT control
        self.ptt = pluto_ptt_control("ip:pluto.local", gpio_pin=0)
        
        # Connect blocks
        self.connect(encoder, pluto_sink)
        
        # Message port for PTT control
        self.message_port_register_in(pmt.intern("ptt_control"))
        self.set_msg_handler(pmt.intern("ptt_control"), self.handle_ptt)
        
    def handle_ptt(self, msg):
        state = pmt.to_bool(msg)
        self.ptt.set_ptt(state)
        
    def start(self):
        gr.top_block.start(self)
        
    def stop(self):
        self.ptt.cleanup()
        gr.top_block.stop(self)
```

#### Alternative: Using IIO Sysfs Interface

For direct GPIO control without libiio:

```python
import os

class pluto_sysfs_gpio:
    def __init__(self, gpio_pin=0):
        self.gpio_pin = gpio_pin
        self.iio_path = "/sys/bus/iio/devices/iio:device0"
        # Note: GPIO access path may vary
        self.gpio_path = f"{self.iio_path}/out_voltage{gpio_pin}_raw"
        
    def set_ptt(self, state):
        """Set PTT via sysfs"""
        try:
            with open(self.gpio_path, "w") as f:
                f.write("1" if state else "0")
        except IOError as e:
            print(f"Failed to set GPIO: {e}")
```

#### Using the Built-in PlutoSDR PTT Control Block

The `gr-packet-protocols` module includes a dedicated `pluto_ptt_control` block that simplifies PTT control for PlutoSDR.

**Prerequisites**: Install `pylibiio` before using this block:
```bash
pip install pylibiio
```

On some systems (e.g., newer Debian/Ubuntu), you may need to use:
```bash
pip install --break-system-packages pylibiio
```

##### Basic Usage in GRC

1. Add the "PlutoSDR PTT Control" block from the Packet Protocols category
2. Configure:
   - **PlutoSDR URI**: Connection string (e.g., "ip:pluto.local")
   - **GPIO Pin**: GPIO pin number (0-7)
   - **TX Delay**: Delay after keying PTT (ms)
   - **TX Tail**: Delay after transmission (ms)
   - **Invert GPIO Logic**: For active-low PTT circuits

3. Connect message ports:
   - Connect a message source to the `ptt_control` message port
   - Send boolean messages: `True` to key PTT, `False` to unkey

##### Python Example

```python
from gnuradio import gr, blocks, packet_protocols, iio
import pmt

class pluto_packet_tx(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        
        # Packet encoder
        encoder = packet_protocols.ax25_encoder(...)
        
        # Adaptive modulator
        modulator = packet_protocols.adaptive_modulator(...)
        
        # PlutoSDR sink
        pluto_sink = iio.pluto_sink(uri="ip:pluto.local", ...)
        
        # PTT Control block
        ptt_control = packet_protocols.pluto_ptt_control(
            pluto_uri="ip:pluto.local",
            gpio_pin=0,
            tx_delay_ms=10,
            tx_tail_ms=10
        )
        
        # Connect blocks
        self.connect(encoder, modulator, pluto_sink)
        
        # Control PTT via message port
        self.message_port_register_out(pmt.intern("ptt_trigger"))
        self.msg_connect((self, "ptt_trigger"), (ptt_control, "ptt_control"))
        
        # Key PTT when starting transmission
        ptt_control.set_ptt(True)
        
    def stop(self):
        # Unkey PTT when stopping
        self.ptt_control.set_ptt(False)
        gr.top_block.stop(self)
```

##### Integration with Half-Duplex Flowgraph

For the `adaptive_half_duplex_example.grc`:

1. Add `pluto_ptt_control` block
2. Connect message ports:
   - Connect selector control logic to `ptt_control` message port
   - When switching to TX mode, send `True`
   - When switching to RX mode, send `False`

3. In Python code:
```python
# In your flowgraph's Python code
def set_tx_mode(self, tx_mode):
    # Update selector
    self.blocks_selector_0.set_input_index(0 if tx_mode else 1)
    
    # Control PTT
    ptt_msg = pmt.from_bool(tx_mode)
    self.msg_connect((self, "ptt_control"), 
                     (self.packet_protocols_pluto_ptt_control_0, "ptt_control"))
    self.packet_protocols_pluto_ptt_control_0.message_port_pub(
        pmt.intern("ptt_control"), ptt_msg)
```

See `examples/pluto_ptt_example.py` for a complete working example.

### gr-uhd (Ettus USRP)

USRP devices provide GPIO control through the UHD API.

#### Basic Usage

```python
from gnuradio import uhd

# Create USRP sink
usrp_sink = uhd.usrp_sink(
    device_args="",
    stream_args=uhd.stream_args('fc32', 'sc16')
)

# Set GPIO for PTT control
# Format: set_gpio_attr(bank, attr, value, mask)
usrp_sink.set_gpio_attr("RXA", "CTRL", 0x01, 0x01)  # Set bit 0
```

#### USRP GPIO Banks

USRP devices have multiple GPIO banks:
- **RXA/RXB**: Receiver GPIO banks
- **TXA/TXB**: Transmitter GPIO banks
- **FP0/FP1**: Front panel GPIO (device-dependent)

#### Complete Example

```python
from gnuradio import gr, blocks, packet_protocols, uhd

class usrp_packet_tx(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        
        # Packet encoder
        encoder = packet_protocols.ax25_encoder(...)
        
        # USRP sink
        usrp_sink = uhd.usrp_sink(
            device_args="",
            stream_args=uhd.stream_args('fc32', 'sc16')
        )
        usrp_sink.set_center_freq(144390000)
        usrp_sink.set_gain(20)
        
        # Configure GPIO for PTT
        # This example uses TXA GPIO bank, bit 0
        usrp_sink.set_gpio_attr("TXA", "CTRL", 0x01, 0x01)
        
        self.connect(encoder, usrp_sink)
```

## Custom GPIO Control

For hardware TNCs requiring direct GPIO control, custom blocks can be created.

### Raspberry Pi GPIO

Using the RPi.GPIO library for Raspberry Pi:

```python
from gnuradio import gr, blocks
import RPi.GPIO as GPIO
import time

class raspberry_pi_ptt(gr.sync_block):
    def __init__(self, gpio_pin=18, tx_delay_ms=10, tx_tail_ms=10):
        gr.sync_block.__init__(
            self,
            name="Raspberry Pi PTT Control",
            in_sig=[gr.sizeof_char],
            out_sig=[gr.sizeof_char]
        )
        self.gpio_pin = gpio_pin
        self.tx_delay_ms = tx_delay_ms / 1000.0
        self.tx_tail_ms = tx_tail_ms / 1000.0
        self.ptt_keyed = False
        self.data_pending = False
        
        # Initialize GPIO
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(gpio_pin, GPIO.OUT)
        GPIO.output(gpio_pin, GPIO.LOW)
        
    def work(self, input_items, output_items):
        in0 = input_items[0]
        out0 = output_items[0]
        
        # Check if we have data to transmit
        has_data = len(in0) > 0
        
        if has_data and not self.ptt_keyed:
            # Key PTT
            GPIO.output(self.gpio_pin, GPIO.HIGH)
            self.ptt_keyed = True
            time.sleep(self.tx_delay_ms)
            self.data_pending = True
            
        if has_data:
            # Copy data to output
            n = min(len(in0), len(out0))
            out0[:n] = in0[:n]
            
            if self.data_pending:
                # Data is being transmitted
                self.data_pending = False
        else:
            # No more data, unkey PTT after tail
            if self.ptt_keyed:
                time.sleep(self.tx_tail_ms)
                GPIO.output(self.gpio_pin, GPIO.LOW)
                self.ptt_keyed = False
            out0[:] = []
            return 0
            
        return len(out0)
        
    def stop(self):
        GPIO.output(self.gpio_pin, GPIO.LOW)
        GPIO.cleanup()
        return True
```

### Linux sysfs GPIO

For generic Linux systems with GPIO support:

```python
import os
import time

class sysfs_gpio_ptt:
    def __init__(self, gpio_number):
        self.gpio_number = gpio_number
        self.gpio_base = f"/sys/class/gpio/gpio{gpio_number}"
        
        # Export GPIO
        if not os.path.exists(self.gpio_base):
            try:
                with open("/sys/class/gpio/export", "w") as f:
                    f.write(str(gpio_number))
                time.sleep(0.1)  # Wait for export
            except IOError:
                raise RuntimeError(f"Failed to export GPIO {gpio_number}")
        
        # Set direction
        with open(f"{self.gpio_base}/direction", "w") as f:
            f.write("out")
            
    def set_ptt(self, state):
        with open(f"{self.gpio_base}/value", "w") as f:
            f.write("1" if state else "0")
            
    def cleanup(self):
        try:
            with open("/sys/class/gpio/unexport", "w") as f:
                f.write(str(self.gpio_number))
        except IOError:
            pass
```

### libgpiod (Modern Linux GPIO)

For systems using libgpiod (recommended for newer Linux kernels):

```python
import gpiod

class libgpiod_ptt:
    def __init__(self, chip_name, line_offset):
        self.chip = gpiod.Chip(chip_name)
        self.line = self.chip.get_line(line_offset)
        self.line.request(consumer="gr-packet-protocols", type=gpiod.LINE_REQ_DIR_OUT)
        
    def set_ptt(self, state):
        self.line.set_value(1 if state else 0)
        
    def cleanup(self):
        self.line.release()
        self.chip.close()
```

## Message-Based Control

For precise timing control, use message passing to coordinate PTT with packet events.

### Tagged Stream Approach

```python
from gnuradio import gr, blocks
import pmt

class tagged_ptt_control(gr.sync_block):
    def __init__(self, gpio_pin=18):
        gr.sync_block.__init__(
            self,
            name="Tagged PTT Control",
            in_sig=[gr.sizeof_char],
            out_sig=[gr.sizeof_char]
        )
        self.gpio_pin = gpio_pin
        # Initialize GPIO...
        
    def work(self, input_items, output_items):
        # Check for tags indicating packet start/end
        tags = self.get_tags_in_window(0, 0, len(input_items[0]))
        
        for tag in tags:
            if pmt.to_python(tag.key) == "packet_start":
                # Key PTT
                GPIO.output(self.gpio_pin, GPIO.HIGH)
            elif pmt.to_python(tag.key) == "packet_end":
                # Unkey PTT
                GPIO.output(self.gpio_pin, GPIO.LOW)
                
        # Pass through data
        output_items[0][:] = input_items[0][:]
        return len(output_items[0])
```

### Message Port Approach

```python
from gnuradio import gr
import pmt

class message_ptt_control(gr.basic_block):
    def __init__(self, gpio_pin=18):
        gr.basic_block.__init__(
            self,
            name="Message PTT Control",
            in_sig=None,
            out_sig=None
        )
        self.gpio_pin = gpio_pin
        self.message_port_register_in(pmt.intern("tx_trigger"))
        self.set_msg_handler(pmt.intern("tx_trigger"), self.handle_tx_trigger)
        # Initialize GPIO...
        
    def handle_tx_trigger(self, msg):
        # Extract packet information from message
        packet_data = pmt.to_python(msg)
        
        # Key PTT
        GPIO.output(self.gpio_pin, GPIO.HIGH)
        time.sleep(0.01)  # TX delay
        
        # Calculate packet duration and wait
        # (implementation depends on packet encoding)
        
        # Unkey PTT
        GPIO.output(self.gpio_pin, GPIO.LOW)
```

## Timing Considerations

Proper PTT timing is critical for reliable packet transmission:

### TX Delay

Time to wait after keying PTT before starting transmission. Allows:
- Transmitter to stabilize
- VCO to lock
- Power amplifier to reach operating level

Typical values: 10-50ms (1-5 units in KISS protocol)

### TX Tail

Time to wait after transmission before unkeying PTT. Ensures:
- Last bits are fully transmitted
- Carrier drops cleanly
- No clipping of final data

Typical values: 10-30ms (1-3 units in KISS protocol)

### Implementation Tips

1. **Use high-resolution timers**: `time.sleep()` may not be precise enough for short delays. Consider `time.perf_counter()` for precise timing.

2. **Coordinate with data stream**: Ensure PTT is keyed before data starts and unkeyed after data completes.

3. **Handle edge cases**: Account for empty packets, rapid successive packets, and flowgraph start/stop events.

## Hardware-Specific Examples

### Dire Wolf TNC

Dire Wolf software TNC typically handles PTT automatically. For hardware TNCs compatible with Dire Wolf:

```python
# Use built-in KISS TNC PTT control
tnc = packet_protocols.kiss_tnc("/dev/ttyUSB0", 9600)
tnc.set_ptt_enabled(True)
```

### Kantronics TNC

Kantronics TNCs use KISS protocol. Use built-in PTT control with appropriate serial port settings.

### Custom Arduino/ESP32 TNC

For custom TNC hardware using Arduino or ESP32:

1. Implement KISS protocol on the microcontroller
2. Use GPIO pin for PTT control
3. Connect to GNU Radio via serial port
4. Use built-in KISS TNC block with PTT control

### BeagleBone Black

BeagleBone Black has extensive GPIO capabilities:

```python
# Use sysfs GPIO or libgpiod
ptt = sysfs_gpio_ptt(gpio_number=60)  # P9_12 on BeagleBone
```

## Troubleshooting

### PTT Not Keying

- Check GPIO pin number and permissions
- Verify hardware connections
- Test GPIO manually before integrating
- Check for conflicting GPIO usage

### Timing Issues

- Increase TX delay if packets are being clipped
- Increase TX tail if final bits are lost
- Use oscilloscope or logic analyzer to verify timing

### SDR-Specific Issues

- Verify GPIO pin assignments for your device
- Check driver support for GPIO control
- Some devices require external PTT control circuits

## References

- [GNU Radio Documentation](https://wiki.gnuradio.org/)
- [Osmocom SDR Wiki](https://osmocom.org/projects/sdr/wiki)
- [UHD Manual](https://files.ettus.com/manual/)
- [Linux GPIO Documentation](https://www.kernel.org/doc/html/latest/driver-api/gpio/)


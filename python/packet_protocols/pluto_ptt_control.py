#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright 2024 gr-packet-protocols
#
# This file is part of gr-packet-protocols
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from gnuradio import gr
import pmt
import threading
import time


class pluto_ptt_control(gr.basic_block):
    """
    PlutoSDR PTT Control Block

    Controls Push-To-Talk (PTT) on PlutoSDR via IIO GPIO pins.
    This block provides message-based PTT control for half-duplex operation.

    Args:
        pluto_uri: PlutoSDR URI (e.g., "ip:pluto.local" or "usb:1.2.5")
        gpio_pin: GPIO pin number to use for PTT (default: 0)
        tx_delay_ms: TX delay in milliseconds (default: 10)
        tx_tail_ms: TX tail in milliseconds (default: 10)
        invert: Invert GPIO logic (True = active low, False = active high) (default: False)
    """

    def __init__(self, pluto_uri="ip:pluto.local", gpio_pin=0,
                 tx_delay_ms=10, tx_tail_ms=10, invert=False):
        gr.basic_block.__init__(
            self,
            name="pluto_ptt_control",
            in_sig=None,
            out_sig=None
        )

        self.pluto_uri = pluto_uri
        self.gpio_pin = gpio_pin
        self.tx_delay_ms = tx_delay_ms / 1000.0
        self.tx_tail_ms = tx_tail_ms / 1000.0
        self.invert = invert
        self.ptt_state = False
        self.lock = threading.Lock()

        # Try to import libiio
        try:
            import iio as libiio
            self.libiio = libiio
            self.iio_available = True
        except ImportError:
            self.iio_available = False
            print("WARNING: libiio Python bindings not available. "
                  "PTT control will not work. Install with: pip install pylibiio")
            self.ctx = None
            self.gpio_attr = None
            return

        # Initialize IIO context and GPIO
        try:
            self.ctx = libiio.Context(pluto_uri)
            if not self.ctx:
                raise RuntimeError(f"Failed to create IIO context for {pluto_uri}")

            # Find GPIO device
            self.gpio = self.ctx.find_device("gpio")
            if not self.gpio:
                raise RuntimeError("GPIO device not found on PlutoSDR")

            # For PlutoSDR, GPIO is typically accessed via device attributes
            # The GPIO pins are controlled via the "out" attribute
            # Check if the device has the required attributes
            if not self.gpio.attrs:
                raise RuntimeError("GPIO device has no attributes")

            # Initialize PTT to RX (unkeyed)
            self._set_gpio(False)

        except Exception as e:
            print(f"WARNING: Failed to initialize PlutoSDR GPIO: {e}")
            self.iio_available = False
            self.ctx = None
            self.gpio = None

        # Register message port for PTT control
        self.message_port_register_in(pmt.intern("ptt_control"))
        self.set_msg_handler(pmt.intern("ptt_control"), self.handle_ptt_message)

        # Register message port for stream-based control (optional)
        self.message_port_register_in(pmt.intern("tx_trigger"))
        self.set_msg_handler(pmt.intern("tx_trigger"), self.handle_tx_trigger)

    def _set_gpio(self, state):
        """
        Internal method to set GPIO pin state using libiio API.
        
        For PlutoSDR, GPIO pins are controlled via device attributes.
        The "out" attribute contains a bitmask where each bit represents a GPIO pin.
        """
        if not self.iio_available or not self.gpio:
            return

        try:
            # Apply inversion if needed
            gpio_value = not state if self.invert else state

            # Read current GPIO value from "out" attribute
            current_value = 0
            try:
                # Access the "out" attribute by name
                out_attr = self.gpio.find_attr("out")
                if out_attr:
                    # Read current value
                    current_str = out_attr.read()
                    if current_str:
                        current_value = int(current_str, 0)  # Allow hex/octal/decimal
            except (ValueError, AttributeError, TypeError) as e:
                # If read fails or returns None, start with 0
                current_value = 0

            # Set or clear the bit for our GPIO pin
            if gpio_value:
                new_value = current_value | (1 << self.gpio_pin)
            else:
                new_value = current_value & ~(1 << self.gpio_pin)

            # Write new value to "out" attribute
            try:
                out_attr = self.gpio.find_attr("out")
                if out_attr:
                    out_attr.write(str(new_value))
                else:
                    raise RuntimeError("GPIO 'out' attribute not found")
            except Exception as e:
                raise RuntimeError(f"Failed to write GPIO value: {e}")

            self.ptt_state = state

        except Exception as e:
            print(f"ERROR: Failed to set GPIO: {e}")
            # Don't update ptt_state if write failed

    def set_ptt(self, state, apply_delay=True):
        """
        Set PTT state

        Args:
            state: True for TX (keyed), False for RX (unkeyed)
            apply_delay: Apply TX delay/tail timing (default: True)
        """
        with self.lock:
            if state == self.ptt_state:
                return  # Already in desired state

            if state:
                # Key PTT (TX mode)
                if apply_delay and self.tx_delay_ms > 0:
                    time.sleep(self.tx_delay_ms)
                self._set_gpio(True)
            else:
                # Unkey PTT (RX mode)
                if apply_delay and self.tx_tail_ms > 0:
                    time.sleep(self.tx_tail_ms)
                self._set_gpio(False)

    def get_ptt(self):
        """Get current PTT state"""
        return self.ptt_state

    def handle_ptt_message(self, msg):
        """Handle PTT control message"""
        try:
            if pmt.is_bool(msg):
                state = pmt.to_bool(msg)
                self.set_ptt(state)
            elif pmt.is_integer(msg):
                state = pmt.to_long(msg) != 0
                self.set_ptt(state)
            elif pmt.is_pair(msg):
                # Assume (state, duration) tuple
                state_pmt = pmt.car(msg)
                if pmt.is_bool(state_pmt):
                    state = pmt.to_bool(state_pmt)
                    self.set_ptt(state)
        except Exception as e:
            print(f"ERROR: Failed to handle PTT message: {e}")

    def handle_tx_trigger(self, msg):
        """Handle TX trigger message (key PTT, wait for duration, unkey)"""
        try:
            if pmt.is_pair(msg):
                # Extract duration from message
                duration_pmt = pmt.cdr(msg)
                if pmt.is_number(duration_pmt):
                    duration = pmt.to_double(duration_pmt)
                    # Key PTT
                    self.set_ptt(True)
                    # Wait for duration
                    time.sleep(duration)
                    # Unkey PTT
                    self.set_ptt(False)
            else:
                # Simple trigger - key and unkey immediately
                self.set_ptt(True)
                time.sleep(0.01)  # Minimal TX time
                self.set_ptt(False)
        except Exception as e:
            print(f"ERROR: Failed to handle TX trigger: {e}")

    def stop(self):
        """Cleanup on stop"""
        if self.ptt_state:
            self.set_ptt(False)
        return True


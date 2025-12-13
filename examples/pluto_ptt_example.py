#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Example: PlutoSDR PTT Control for Half-Duplex Operation
#
# This example demonstrates how to use the PlutoSDR PTT control block
# in a half-duplex packet radio flowgraph.

from gnuradio import gr, blocks, packet_protocols
from gnuradio import iio
import pmt
import time
import threading


class pluto_half_duplex_tx(gr.top_block):
    """
    Half-duplex TX example with PlutoSDR PTT control
    """

    def __init__(self):
        gr.top_block.__init__(self, "PlutoSDR Half-Duplex TX")

        # Packet encoder
        encoder = packet_protocols.ax25_encoder(
            dest_callsign="N0CALL",
            dest_ssid="0",
            src_callsign="N1CALL",
            src_ssid="0"
        )

        # Adaptive modulator
        modulator = packet_protocols.adaptive_modulator(
            initial_mode=packet_protocols.modulation_mode_t.MODE_4FSK,
            samples_per_symbol=2
        )

        # PlutoSDR sink
        pluto_sink = iio.pluto_sink(
            uri="ip:pluto.local",
            buffer_size=32768,
            cyclic=False
        )
        pluto_sink.set_params(
            144390000,  # 2m band (Hz)
            9600000,    # Sample rate (Hz)
            20000000,   # Bandwidth (Hz)
            0,          # RF port (0 = A, 1 = B)
            -89.75,     # Attenuation (dB)
            True        # Filter auto
        )

        # PTT Control
        ptt_control = packet_protocols.pluto_ptt_control(
            pluto_uri="ip:pluto.local",
            gpio_pin=0,
            tx_delay_ms=10,
            tx_tail_ms=10,
            invert=False
        )

        # Data source
        data_source = blocks.vector_source_b(
            [0x48, 0x65, 0x6C, 0x6C, 0x6F],  # "Hello"
            True,  # Repeat
            1
        )

        # Throttle
        throttle = blocks.throttle(gr.sizeof_char, 9600, True)

        # Connect blocks
        self.connect(data_source, throttle, encoder, modulator, pluto_sink)

        # Message port for PTT control
        # Key PTT when data starts, unkey when done
        self.message_port_register_out(pmt.intern("ptt_trigger"))
        self.connect((encoder, 0), (modulator, 0))

        # Store references for PTT control
        self.ptt_control = ptt_control
        self.encoder = encoder

    def start(self):
        """Start flowgraph and key PTT"""
        gr.top_block.start(self)
        # Key PTT for transmission
        self.ptt_control.set_ptt(True)

    def stop(self):
        """Stop flowgraph and unkey PTT"""
        # Unkey PTT
        self.ptt_control.set_ptt(False)
        gr.top_block.stop(self)


class pluto_half_duplex_rx(gr.top_block):
    """
    Half-duplex RX example with PlutoSDR
    """

    def __init__(self):
        gr.top_block.__init__(self, "PlutoSDR Half-Duplex RX")

        # PlutoSDR source
        pluto_source = iio.pluto_source(
            uri="ip:pluto.local",
            buffer_size=32768
        )
        pluto_source.set_params(
            144390000,  # 2m band (Hz)
            9600000,    # Sample rate (Hz)
            20000000,   # Bandwidth (Hz)
            0,          # RF port (0 = A, 1 = B)
            64,         # Gain (dB)
            True        # Filter auto
        )

        # Demodulator (GFSK for 4FSK example)
        demodulator = blocks.digital_gfsk_demod(
            samples_per_symbol=2,
            sensitivity=1.0,
            gain_mu=0.175,
            mu=0.5,
            omega_relative_limit=0.005,
            freq_error=0.0,
            verbose=False,
            log=False
        )

        # Packet decoder
        decoder = packet_protocols.ax25_decoder()

        # Sink
        sink = blocks.null_sink(gr.sizeof_char)

        # Connect blocks
        self.connect(pluto_source, demodulator, decoder, sink)


def main():
    """Main function"""
    print("PlutoSDR Half-Duplex Example")
    print("============================")
    print("\nThis example demonstrates:")
    print("1. TX with automatic PTT control")
    print("2. RX operation")
    print("\nNote: This is a simplified example.")
    print("For full half-duplex operation, use the adaptive_half_duplex_example.grc")
    print("and add the pluto_ptt_control block with message port connections.")

    # Example: TX
    print("\nStarting TX example...")
    tb_tx = pluto_half_duplex_tx()
    tb_tx.start()
    time.sleep(2)  # Transmit for 2 seconds
    tb_tx.stop()
    tb_tx.wait()
    print("TX example complete.")

    # Example: RX
    print("\nStarting RX example...")
    tb_rx = pluto_half_duplex_rx()
    tb_rx.start()
    time.sleep(5)  # Receive for 5 seconds
    tb_rx.stop()
    tb_rx.wait()
    print("RX example complete.")


if __name__ == "__main__":
    main()


#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Adaptive Speed Example - Converting Basic Flowgraph to Adaptive
# Author: gr-packet-protocols
# Description: Example showing how to make a basic flowgraph (Vector Source -> Throttle -> AX.25 Encoder -> KISS TNC -> Null Sink) speed adaptive
# GNU Radio version: 3.10+

from gnuradio import gr
from gnuradio import blocks
from gnuradio import digital
from gnuradio.analog import cpm
from gnuradio import packet_protocols
from gnuradio.packet_protocols import modulation_switch
import threading
import time
import sys
import signal


class adaptive_speed_flowgraph(gr.top_block):
    """
    Example flowgraph showing how to make a basic packet flowgraph speed adaptive.
    
    Original flowgraph:
    [Vector Source] -> [Throttle] -> [AX.25 Encoder] -> [KISS TNC] -> [Null Sink]
    
    Adaptive flowgraph:
    [Vector Source] -> [Throttle] -> [AX.25 Encoder] -> [Link Quality Monitor] -> 
    [Multiple Modulators] -> [Modulation Switch] -> [Null Sink]
    """
    
    def __init__(self):
        gr.top_block.__init__(self, "Adaptive Speed Example", catch_exceptions=True)
        
        # ============================================================
        # Your original blocks
        # ============================================================
        
        # Vector source with test data
        self.vector_source = blocks.vector_source_b(
            [0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64],  # "Hello World"
            True,  # repeat
            1,    # vlen
            []    # tags
        )
        
        # Throttle to control data rate
        self.throttle = blocks.throttle(
            gr.sizeof_char * 1,
            9600,  # samples per second
            True
        )
        
        # AX.25 Encoder
        self.ax25_encoder = packet_protocols.ax25_encoder(
            'N0CALL',  # source call
            '0',       # source SSID
            'N1CALL',  # destination call
            '0',       # destination SSID
            '',        # path (digipeaters)
            False,     # repeat flag
            False      # command/response
        )
        
        # ============================================================
        # NEW: Adaptive components
        # ============================================================
        
        # Link Quality Monitor - tracks SNR, BER, frame error rate
        self.quality_monitor = packet_protocols.link_quality_monitor(
            alpha=0.1,        # Smoothing factor (0.0-1.0, higher = faster response)
            update_period=1000  # Update every N samples
        )
        
        # Adaptive Rate Control - automatically selects best modulation mode
        self.rate_control = packet_protocols.adaptive_rate_control(
            initial_mode=packet_protocols.modulation_mode_t.MODE_4FSK,
            enable_adaptation=True,   # Enable automatic mode switching
            hysteresis_db=2.0        # Prevent rapid switching (2 dB margin)
        )
        
        # Create modulation blocks for different modes
        samples_per_symbol = 2
        
        # 2FSK - Most robust, lowest rate (1200 bps)
        self.mod_2fsk = digital.gfsk_mod(
            samples_per_symbol=samples_per_symbol,
            sensitivity=1.0,
            bt=0.35
        )
        
        # 4FSK - Good balance (2400 bps)
        self.mod_4fsk = cpm.cpmmod_bc(
            type=cpm.CPM_LREC,
            h=0.5,
            samples_per_sym=samples_per_symbol,
            L=4,
            beta=0.3
        )
        
        # 8FSK - Higher rate (3600 bps)
        self.mod_8fsk = cpm.cpmmod_bc(
            type=cpm.CPM_LREC,
            h=0.5,
            samples_per_sym=samples_per_symbol,
            L=4,
            beta=0.3
        )
        
        # QPSK - Good PSK option (2400 bps)
        self.mod_qpsk = digital.psk.psk_mod(
            constellation_points=4,
            mod_code=digital.GRAY_CODE,
            differential=False
        )
        
        # Mode to index mapping for modulation switch
        self.mode_to_index = {
            packet_protocols.modulation_mode_t.MODE_2FSK: 0,
            packet_protocols.modulation_mode_t.MODE_4FSK: 1,
            packet_protocols.modulation_mode_t.MODE_8FSK: 2,
            packet_protocols.modulation_mode_t.MODE_QPSK: 3,
        }
        
        # Modulation Switch - switches between modulator outputs
        self.mod_switch = modulation_switch(
            self.rate_control,
            self.mode_to_index,
            num_inputs=4
        )
        
        # ============================================================
        # Output sink
        # ============================================================
        
        # Null sink for testing (replace with actual sink in real application)
        # Note: Modulators output complex samples, so use complex sink
        self.null_sink = blocks.null_sink(gr.sizeof_gr_complex * 1)
        
        # If you want to use KISS TNC, note that it expects bytes, not complex samples
        # You would need to add a complex-to-byte converter or use a different approach
        # self.kiss_tnc = packet_protocols.kiss_tnc('/dev/ttyUSB0', 9600, False)
        
        # ============================================================
        # Connect blocks
        # ============================================================
        
        # Original path: source -> throttle -> encoder
        self.connect(self.vector_source, self.throttle)
        self.connect(self.throttle, self.ax25_encoder)
        
        # NEW: encoder -> quality monitor
        self.connect(self.ax25_encoder, self.quality_monitor)
        
        # NEW: quality monitor -> all modulators (they all receive same input)
        self.connect((self.quality_monitor, 0), (self.mod_2fsk, 0))
        self.connect((self.quality_monitor, 0), (self.mod_4fsk, 0))
        self.connect((self.quality_monitor, 0), (self.mod_8fsk, 0))
        self.connect((self.quality_monitor, 0), (self.mod_qpsk, 0))
        
        # NEW: all modulators -> modulation switch
        self.connect((self.mod_2fsk, 0), (self.mod_switch, 0))
        self.connect((self.mod_4fsk, 0), (self.mod_switch, 1))
        self.connect((self.mod_8fsk, 0), (self.mod_switch, 2))
        self.connect((self.mod_qpsk, 0), (self.mod_switch, 3))
        
        # NEW: modulation switch -> output
        self.connect((self.mod_switch, 0), self.null_sink)
        
        # ============================================================
        # Setup quality updates
        # ============================================================
        
        self._setup_quality_updates()
    
    def _setup_quality_updates(self):
        """
        Setup periodic quality updates to drive adaptation.
        
        In a real system, these metrics would come from:
        - SNR estimator (e.g., digital.probe_mpsk_snr_est_c) in receive path
        - BER measurement from frame validation
        - Frame error rate from successful/failed frame counts
        """
        def update_quality():
            import random
            while True:
                time.sleep(1.0)  # Update every second
                
                # Simulate varying SNR (in real system, get from SNR estimator)
                snr_db = 10.0 + random.uniform(-5, 10)  # SNR between 5-20 dB
                
                # Simulate BER (inversely related to SNR)
                ber = max(0.0, min(0.01, 0.001 * (20 - snr_db) / 10))
                
                # Calculate quality score (0.0-1.0)
                quality_score = min(1.0, max(0.0, (snr_db - 5) / 20))
                
                # Update link quality monitor
                self.quality_monitor.update_snr(snr_db)
                self.quality_monitor.update_ber(ber)
                
                # Update adaptive rate control (this triggers mode changes)
                self.rate_control.update_quality(
                    snr_db, ber, quality_score
                )
                
                # Get current mode and rate
                current_mode = self.rate_control.get_modulation_mode()
                current_rate = self.rate_control.get_data_rate()
                
                # Print status
                mode_name = str(current_mode).split('.')[-1] if hasattr(current_mode, '__str__') else str(current_mode)
                print(f"SNR: {snr_db:5.1f} dB | BER: {ber:.4f} | Mode: {mode_name:12s} | Rate: {current_rate:5d} bps")
                
                # Send mode update message to modulation switch
                from gnuradio import pmt
                self.mod_switch.message_port_pub(
                    pmt.intern("mode_update"),
                    pmt.PMT_NIL
                )
        
        # Start quality update thread
        quality_thread = threading.Thread(target=update_quality, daemon=True)
        quality_thread.start()
        print("Quality monitoring thread started")
        print("=" * 70)
        print("SNR (dB) | BER      | Mode         | Rate (bps)")
        print("=" * 70)


def main(top_block_cls=adaptive_speed_flowgraph, options=None):
    """Run the flowgraph"""
    tb = top_block_cls()
    
    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()
        sys.exit(0)
    
    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)
    
    tb.start()
    print("Flowgraph started. Press Ctrl+C to stop.")
    print("")
    
    try:
        tb.wait()
    except KeyboardInterrupt:
        pass
    finally:
        tb.stop()
        tb.wait()


if __name__ == '__main__':
    main()


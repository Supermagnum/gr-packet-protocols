#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Half-Duplex Adaptive Modulation System
# Author: gr-packet-protocols
# Description: Half-duplex adaptive system with TX/RX switching, link quality monitoring,
#              adaptive rate control, modulation negotiation, and quality feedback
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
from gnuradio import pmt


class adaptive_half_duplex_flowgraph(gr.top_block):
    """
    Half-duplex adaptive modulation system with TX/RX switching:
    - TX Mode: Source -> Encoder -> Link Quality Monitor -> Adaptive Modulator -> Radio
    - RX Mode: Radio -> Demodulator -> Decoder -> Link Quality Monitor
    - PTT Control: Switches between TX and RX modes
    - Adaptive Rate Control: Automatically selects modulation mode
    - Modulation Negotiation: Coordinates mode changes between stations
    - KISS TNC: Handles negotiation frames via message ports
    - Quality Feedback: Bidirectional quality metric exchange
    """
    
    def __init__(self):
        gr.top_block.__init__(self, "Half-Duplex Adaptive System", catch_exceptions=True)
        
        # Configuration
        self.station_id = "N0CALL"
        self.remote_station_id = "N1CALL"
        self.samp_rate = 96000
        self.samples_per_symbol = 2
        
        # PTT State (False = RX mode, True = TX mode)
        self.ptt_state = False
        self.ptt_lock = threading.Lock()
        
        # ========================================================================
        # TX PATH
        # ========================================================================
        
        # TX Data Source
        self.tx_source = blocks.vector_source_b(
            [0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64],  # "Hello World"
            True, 1, []
        )
        
        # TX Throttle
        self.tx_throttle = blocks.throttle(
            gr.sizeof_char * 1,
            9600,  # samples per second
            True
        )
        
        # TX AX.25 Encoder
        self.tx_encoder = packet_protocols.ax25_encoder(
            self.station_id, '0',  # source
            self.remote_station_id, '0',  # destination
            '', False, False
        )
        
        # TX Link Quality Monitor
        self.tx_quality_monitor = packet_protocols.link_quality_monitor(
            alpha=0.1,
            update_period=1000
        )
        
        # TX Adaptive Modulator
        self.tx_adaptive_mod = packet_protocols.adaptive_modulator(
            initial_mode=packet_protocols.modulation_mode_t.MODE_4FSK,
            samples_per_symbol=self.samples_per_symbol,
            enable_adaptation=True,
            hysteresis_db=2.0
        )
        
        # ========================================================================
        # RX PATH
        # ========================================================================
        
        # RX Input Source (replace with USRP source for SDR)
        self.rx_source = blocks.file_source(
            gr.sizeof_gr_complex * 1,
            '/tmp/radio_input.cfile',
            True
        )
        
        # RX Demodulators (create all modes, switch between them)
        self.rx_demod_2fsk = digital.gfsk_demod(
            samples_per_symbol=self.samples_per_symbol,
            sensitivity=1.0,
            gain_mu=0.175,
            mu=0.5,
            omega_relative_limit=0.005,
            freq_error=0.0,
            verbose=False,
            log=False
        )
        
        # RX SNR Estimator
        self.rx_snr_estimator = digital.probe_mpsk_snr_est_c(
            type=digital.SNR_EST_SIMPLE,
            msg_nsamples=1000,
            alpha=0.001
        )
        
        # RX AX.25 Decoder
        self.rx_decoder = packet_protocols.ax25_decoder()
        
        # RX Link Quality Monitor
        self.rx_quality_monitor = packet_protocols.link_quality_monitor(
            alpha=0.1,
            update_period=1000
        )
        
        # RX Output Sink
        self.rx_sink = blocks.null_sink(gr.sizeof_char * 1)
        
        # ========================================================================
        # TX/RX SWITCHING
        # ========================================================================
        
        # TX/RX Mode Selector
        # Input 0: TX path (adaptive modulator output)
        # Input 1: RX path (from radio, but we don't route RX to output in TX mode)
        # Output: Selected path to radio
        self.tx_rx_selector = blocks.selector(
            gr.sizeof_gr_complex,
            0,  # Start in TX mode (index 0)
            0   # Output index
        )
        
        # RX Input Selector
        # Input 0: From radio (RX mode)
        # Input 1: Null source (TX mode - no RX processing)
        self.null_source = blocks.null_source(gr.sizeof_gr_complex * 1)
        self.rx_input_selector = blocks.selector(
            gr.sizeof_gr_complex,
            0,  # Start in RX mode (index 0)
            0   # Output index
        )
        
        # Radio Output Sink (replace with USRP sink for SDR)
        self.radio_output = blocks.file_sink(
            gr.sizeof_gr_complex * 1,
            '/tmp/radio_output.cfile',
            False
        )
        
        # ========================================================================
        # ADAPTIVE CONTROL
        # ========================================================================
        
        # Adaptive Rate Control
        self.rate_control = packet_protocols.adaptive_rate_control(
            initial_mode=packet_protocols.modulation_mode_t.MODE_4FSK,
            enable_adaptation=True,
            hysteresis_db=2.0
        )
        
        # Get rate control from adaptive modulator (they should share the same instance)
        self.shared_rate_control = self.tx_adaptive_mod.get_rate_control()
        
        # ========================================================================
        # MODULATION NEGOTIATION
        # ========================================================================
        
        # Modulation Negotiation
        self.negotiator = packet_protocols.modulation_negotiation(
            station_id=self.station_id,
            supported_modes=[
                packet_protocols.modulation_mode_t.MODE_2FSK,
                packet_protocols.modulation_mode_t.MODE_4FSK,
                packet_protocols.modulation_mode_t.MODE_8FSK,
                packet_protocols.modulation_mode_t.MODE_QPSK,
            ],
            negotiation_timeout_ms=5000
        )
        
        # ========================================================================
        # KISS TNC (for negotiation frames and PTT control)
        # ========================================================================
        
        # KISS TNC - handles negotiation frames and PTT control
        self.kiss_tnc = packet_protocols.kiss_tnc(
            device='/dev/null',  # Change to actual device
            baud_rate=9600,
            hardware_flow_control=False
        )
        
        # ========================================================================
        # CONNECT BLOCKS
        # ========================================================================
        
        # TX Path
        self.connect(self.tx_source, self.tx_throttle)
        self.connect(self.tx_throttle, self.tx_encoder)
        self.connect(self.tx_encoder, self.tx_quality_monitor)
        self.connect(self.tx_quality_monitor, self.tx_adaptive_mod)
        self.connect(self.tx_adaptive_mod, (self.tx_rx_selector, 0))
        
        # RX Path
        self.connect(self.rx_source, (self.rx_input_selector, 0))
        self.connect(self.null_source, (self.rx_input_selector, 1))
        self.connect(self.rx_input_selector, self.rx_demod_2fsk)
        self.connect(self.rx_input_selector, self.rx_snr_estimator)  # Parallel for SNR
        self.connect(self.rx_demod_2fsk, self.rx_decoder)
        self.connect(self.rx_decoder, self.rx_quality_monitor)
        self.connect(self.rx_quality_monitor, self.rx_sink)
        
        # Radio Interface
        # In TX mode: selector outputs TX signal
        # In RX mode: selector outputs nothing (or we could route RX back, but typically not)
        self.connect(self.tx_rx_selector, self.radio_output)
        
        # ========================================================================
        # MESSAGE PORT CONNECTIONS
        # ========================================================================
        
        # Connect KISS TNC negotiation_out to Modulation Negotiation negotiation_in
        self.msg_connect(
            self.kiss_tnc, "negotiation_out",
            self.negotiator, "negotiation_in"
        )
        
        # Connect SNR estimator to quality update handler
        self.rx_snr_estimator.message_port_register_out(pmt.intern("snr"))
        self.msg_connect(
            self.rx_snr_estimator, "snr",
            self, pmt.intern("snr_update")
        )
        
        # Register message handler for SNR updates
        self.message_port_register_in(pmt.intern("snr_update"))
        self.set_msg_handler(pmt.intern("snr_update"), self.handle_snr_update)
        
        # ========================================================================
        # SETUP NEGOTIATION CALLBACK
        # ========================================================================
        
        # Enable automatic negotiation
        try:
            self.negotiator.set_auto_negotiation_enabled(True, self.shared_rate_control)
        except AttributeError:
            print("Warning: set_auto_negotiation_enabled not available")
        
        # ========================================================================
        # SETUP QUALITY UPDATES AND FEEDBACK
        # ========================================================================
        
        self._setup_quality_updates()
        self._setup_quality_feedback()
        self._setup_ptt_control()
        
        print("Half-duplex adaptive system initialized")
        print(f"Station ID: {self.station_id}")
        print(f"Remote Station ID: {self.remote_station_id}")
        print("Initial mode: RX (listening)")
        print("=" * 70)
    
    def handle_snr_update(self, msg):
        """Handle SNR update from estimator"""
        try:
            snr_db = pmt.to_double(msg)
            # Only update if in RX mode
            with self.ptt_lock:
                if not self.ptt_state:  # RX mode
                    self.rx_quality_monitor.update_snr(snr_db)
        except Exception as e:
            print(f"Error handling SNR update: {e}")
    
    def set_ptt(self, state):
        """Set PTT state (True = TX mode, False = RX mode)"""
        with self.ptt_lock:
            if self.ptt_state != state:
                self.ptt_state = state
                
                # Update selectors
                if state:  # TX mode
                    self.tx_rx_selector.set_input_index(0)  # Select TX path
                    self.rx_input_selector.set_input_index(1)  # Select null (disable RX)
                    print("PTT: TX mode (transmitting)")
                else:  # RX mode
                    self.tx_rx_selector.set_input_index(1)  # Select RX path (or null)
                    self.rx_input_selector.set_input_index(0)  # Select RX input
                    print("PTT: RX mode (receiving)")
    
    def _setup_ptt_control(self):
        """Setup PTT control logic"""
        def ptt_control_loop():
            """Simulate PTT control - in real system, this would be triggered by data"""
            while True:
                time.sleep(10.0)  # Simulate periodic transmission
                
                # Key PTT (TX mode)
                self.set_ptt(True)
                time.sleep(2.0)  # Transmit for 2 seconds
                
                # Unkey PTT (RX mode)
                self.set_ptt(False)
        
        # Start PTT control thread
        ptt_thread = threading.Thread(target=ptt_control_loop, daemon=True)
        ptt_thread.start()
        print("PTT control thread started")
    
    def _setup_quality_updates(self):
        """Setup periodic quality updates to drive adaptation"""
        def update_quality():
            import random
            while True:
                time.sleep(1.0)  # Update every second
                
                # Only update quality in RX mode
                with self.ptt_lock:
                    if self.ptt_state:  # TX mode - skip quality updates
                        continue
                
                # Get quality metrics from RX quality monitor
                snr_db = self.rx_quality_monitor.get_snr()
                ber = self.rx_quality_monitor.get_ber()
                quality_score = self.rx_quality_monitor.get_quality_score()
                
                # If no measurements yet, simulate
                if snr_db == 0.0:
                    snr_db = 10.0 + random.uniform(-5, 10)
                    ber = max(0.0, min(0.01, 0.001 * (20 - snr_db) / 10))
                    quality_score = min(1.0, max(0.0, (snr_db - 5) / 20))
                    self.rx_quality_monitor.update_snr(snr_db)
                    self.rx_quality_monitor.update_ber(ber)
                
                # Update adaptive rate control
                self.shared_rate_control.update_quality(
                    snr_db, ber, quality_score
                )
                
                # Get current mode
                current_mode = self.shared_rate_control.get_modulation_mode()
                current_rate = self.shared_rate_control.get_data_rate()
                
                # Update adaptive modulator mode
                self.tx_adaptive_mod.set_modulation_mode(current_mode)
                
                # Print status
                mode_name = str(current_mode).split('.')[-1] if hasattr(current_mode, '__str__') else str(current_mode)
                with self.ptt_lock:
                    mode_str = "TX" if self.ptt_state else "RX"
                print(f"[{mode_str}] SNR: {snr_db:5.1f} dB | BER: {ber:.4f} | Mode: {mode_name:12s} | Rate: {current_rate:5d} bps")
        
        # Start quality update thread
        quality_thread = threading.Thread(target=update_quality, daemon=True)
        quality_thread.start()
        print("Quality monitoring thread started")
    
    def _setup_quality_feedback(self):
        """Setup periodic quality feedback transmission to remote station"""
        def send_quality_feedback():
            while True:
                time.sleep(5.0)  # Send every 5 seconds
                
                # Only send feedback in RX mode (when we have valid measurements)
                with self.ptt_lock:
                    if self.ptt_state:  # TX mode - skip feedback
                        continue
                
                try:
                    # Get quality metrics
                    snr = self.rx_quality_monitor.get_snr()
                    ber = self.rx_quality_monitor.get_ber()
                    quality = self.rx_quality_monitor.get_quality_score()
                    
                    # Send quality feedback to remote station
                    if snr > 0.0:  # Only send if we have valid measurements
                        # Key PTT briefly to send feedback
                        self.set_ptt(True)
                        self.negotiator.send_quality_feedback(
                            self.remote_station_id,
                            snr, ber, quality
                        )
                        time.sleep(0.1)  # Brief transmission
                        self.set_ptt(False)
                        print(f"Sent quality feedback to {self.remote_station_id}: SNR={snr:.1f} dB, BER={ber:.4f}, Quality={quality:.2f}")
                except Exception as e:
                    print(f"Error sending quality feedback: {e}")
        
        # Start quality feedback thread
        feedback_thread = threading.Thread(target=send_quality_feedback, daemon=True)
        feedback_thread.start()
        print("Quality feedback thread started")
    
    def record_frame_success(self):
        """Call this when a frame is successfully decoded"""
        with self.ptt_lock:
            if not self.ptt_state:  # Only in RX mode
                self.rx_quality_monitor.record_frame_success()
    
    def record_frame_error(self):
        """Call this when a frame fails to decode"""
        with self.ptt_lock:
            if not self.ptt_state:  # Only in RX mode
                self.rx_quality_monitor.record_frame_error()


def main(top_block_cls=adaptive_half_duplex_flowgraph, options=None):
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
    print("=" * 70)
    print("Mode | SNR (dB) | BER      | Mode         | Rate (bps)")
    print("=" * 70)
    
    try:
        tb.wait()
    except KeyboardInterrupt:
        pass
    finally:
        tb.stop()
        tb.wait()


if __name__ == '__main__':
    main()


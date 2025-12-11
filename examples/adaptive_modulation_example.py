#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Adaptive Modulation Example
# Author: gr-packet-protocols
# Description: Complete example showing adaptive modulation with link quality monitoring
# GNU Radio version: 3.10.12.0

from PyQt5 import Qt
from gnuradio import qtgui
from gnuradio import blocks
from gnuradio import gr
from gnuradio import digital
from gnuradio.analog import cpm
from gnuradio.filter import firdes
from gnuradio.fft import window
import sys
import signal
from PyQt5 import Qt
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import packet_protocols
import threading
import time


# Note: This controller class is not needed when using modulation_switch block
# It's kept for reference but not used in this example
class adaptive_modulation_controller:
    """
    Controller class that manages modulation mode switching.
    This monitors the adaptive_rate_control and switches between
    modulation blocks using a selector.
    """
    def __init__(self, rate_control, selector, mod_blocks, mode_to_index):
        self.rate_control = rate_control
        self.selector = selector
        self.mod_blocks = mod_blocks
        self.mode_to_index = mode_to_index
        self.running = False
        self.thread = None
        self.last_mode = None

    def start(self):
        """Start monitoring mode changes"""
        self.running = True
        self.last_mode = self.rate_control.get_modulation_mode()
        self.thread = threading.Thread(target=self._monitor_loop, daemon=True)
        self.thread.start()

    def stop(self):
        """Stop monitoring"""
        self.running = False
        if self.thread:
            self.thread.join(timeout=1.0)

    def _monitor_loop(self):
        """Monitor mode changes and update selector"""
        while self.running:
            time.sleep(0.1)  # Check every 100ms
            try:
                current_mode = self.rate_control.get_modulation_mode()
                if current_mode != self.last_mode:
                    # Mode changed - update selector
                    if current_mode in self.mode_to_index:
                        index = self.mode_to_index[current_mode]
                        # Update selector input index
                        # Note: This requires selector to support set_input_index
                        # or we need to use message passing
                        print(f"Mode changed to: {current_mode}, index: {index}")
                        self.last_mode = current_mode
            except Exception as e:
                print(f"Error in monitor loop: {e}")


class top_block(gr.top_block, Qt.QWidget):
    def __init__(self):
        gr.top_block.__init__(self, "Adaptive Modulation Example", catch_exceptions=True)
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Adaptive Modulation Example")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except BaseException as exc:
            print(f"Qt GUI: Could not set Icon: {str(exc)}", file=sys.stderr)

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 9600
        self.samples_per_symbol = samples_per_symbol = 2

        ##################################################
        # Blocks
        ##################################################

        # Packet encoder
        self.packet_protocols_ax25_encoder_0 = packet_protocols.ax25_encoder(
            'N0CALL', '0', 'N0CALL', '0', '', False, False
        )

        # Link quality monitor
        self.packet_protocols_link_quality_monitor_0 = packet_protocols.link_quality_monitor(
            alpha=0.1,
            update_period=1000
        )

        # Adaptive rate control
        self.packet_protocols_adaptive_rate_control_0 = packet_protocols.adaptive_rate_control(
            initial_mode=packet_protocols.modulation_mode_t.MODE_4FSK,
            enable_adaptation=True,
            hysteresis_db=2.0
        )

        # SNR estimator (from GNU Radio digital module)
        self.digital_probe_mpsk_snr_est_c_0 = digital.probe_mpsk_snr_est_c(
            type=digital.SNR_EST_SIMPLE,
            msg_nsamples=1000,
            alpha=0.001
        )

        # Create modulation blocks for each mode
        self.mod_2fsk = digital.gfsk_mod(
            samples_per_symbol=samples_per_symbol,
            sensitivity=1.0,
            bt=0.35
        )
        self.mod_4fsk = cpm.cpmmod_bc(
            type=cpm.CPM_LREC,
            h=0.5,
            samples_per_sym=samples_per_symbol,
            L=4,
            beta=0.3
        )
        self.mod_8fsk = cpm.cpmmod_bc(
            type=cpm.CPM_LREC,
            h=0.5,
            samples_per_sym=samples_per_symbol,
            L=4,
            beta=0.3
        )
        self.mod_qpsk = digital.psk.psk_mod(
            constellation_points=4,
            mod_code=digital.GRAY_CODE,
            differential=False
        )

        # Use custom modulation switch block instead of selector
        # This provides better control and message-based updates
        from gnuradio.packet_protocols import modulation_switch
        
        self.blocks_modulation_switch_0 = modulation_switch(
            self.packet_protocols_adaptive_rate_control_0,
            self.mode_to_index,
            num_inputs=4
        )

        # Mode to index mapping
        self.mode_to_index = {
            packet_protocols.modulation_mode_t.MODE_2FSK: 0,
            packet_protocols.modulation_mode_t.MODE_4FSK: 1,
            packet_protocols.modulation_mode_t.MODE_8FSK: 2,
            packet_protocols.modulation_mode_t.MODE_QPSK: 3,
        }

        self.mod_blocks = {
            packet_protocols.modulation_mode_t.MODE_2FSK: self.mod_2fsk,
            packet_protocols.modulation_mode_t.MODE_4FSK: self.mod_4fsk,
            packet_protocols.modulation_mode_t.MODE_8FSK: self.mod_8fsk,
            packet_protocols.modulation_mode_t.MODE_QPSK: self.mod_qpsk,
        }

        # Test data source
        self.blocks_vector_source_x_0 = blocks.vector_source_b(
            [0x48, 0x65, 0x6C, 0x6C, 0x6F],  # "Hello"
            True, 1, []
        )

        # Throttle
        self.blocks_throttle_0 = blocks.throttle(
            gr.sizeof_char * 1, samp_rate, True
        )

        # Null sink for output (replace with actual sink in real application)
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_gr_complex * 1)

        ##################################################
        # Connections
        ##################################################

        # Data path: source -> throttle -> encoder -> quality monitor -> modulators -> selector -> output
        self.connect((self.blocks_vector_source_x_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.packet_protocols_ax25_encoder_0, 0))
        self.connect((self.packet_protocols_ax25_encoder_0, 0), (self.packet_protocols_link_quality_monitor_0, 0))

        # Connect quality monitor output to all modulators
        self.connect((self.packet_protocols_link_quality_monitor_0, 0), (self.mod_2fsk, 0))
        self.connect((self.packet_protocols_link_quality_monitor_0, 0), (self.mod_4fsk, 0))
        self.connect((self.packet_protocols_link_quality_monitor_0, 0), (self.mod_8fsk, 0))
        self.connect((self.packet_protocols_link_quality_monitor_0, 0), (self.mod_qpsk, 0))

        # Connect modulators to modulation switch inputs
        self.connect((self.mod_2fsk, 0), (self.blocks_modulation_switch_0, 0))
        self.connect((self.mod_4fsk, 0), (self.blocks_modulation_switch_0, 1))
        self.connect((self.mod_8fsk, 0), (self.blocks_modulation_switch_0, 2))
        self.connect((self.mod_qpsk, 0), (self.blocks_modulation_switch_0, 3))

        # Connect modulation switch to output
        self.connect((self.blocks_modulation_switch_0, 0), (self.blocks_null_sink_0, 0))

        # Setup mode update messages for modulation switch
        self._setup_mode_updates()

        # Simulate quality updates (in real application, this would come from SNR estimator)
        self._setup_quality_updates()

    def _setup_mode_updates(self):
        """Setup periodic mode update messages"""
        pass  # Mode updates are handled in quality update loop

    def _setup_quality_updates(self):
        """Setup periodic quality updates to trigger mode adaptation"""
        def update_quality():
            # Simulate varying SNR
            import random
            while True:
                time.sleep(1.0)  # Update every second
                snr_db = 10.0 + random.uniform(-5, 10)  # SNR between 5-20 dB
                ber = max(0.0, min(0.01, 0.001 * (20 - snr_db) / 10))  # BER inversely related to SNR
                quality_score = min(1.0, max(0.0, (snr_db - 5) / 20))  # Quality score 0-1

                # Update link quality monitor
                self.packet_protocols_link_quality_monitor_0.update_snr(snr_db)
                self.packet_protocols_link_quality_monitor_0.update_ber(ber)

                # Update adaptive rate control
                self.packet_protocols_adaptive_rate_control_0.update_quality(
                    snr_db, ber, quality_score
                )

                # Get current mode
                current_mode = self.packet_protocols_adaptive_rate_control_0.get_modulation_mode()
                current_rate = self.packet_protocols_adaptive_rate_control_0.get_data_rate()

                print(f"SNR: {snr_db:.1f} dB, BER: {ber:.4f}, Mode: {current_mode}, Rate: {current_rate} bps")

                # Send mode update message to modulation switch
                from gnuradio import pmt
                self.blocks_modulation_switch_0.message_port_pub(
                    pmt.intern("mode_update"),
                    pmt.PMT_NIL
                )

        # Start quality update thread
        quality_thread = threading.Thread(target=update_quality, daemon=True)
        quality_thread.start()

    def closeEvent(self, event):
        self.settings = Qt.QSettings("gnuradio/flowgraphs", "top_block")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()
        event.accept()


def main(top_block_cls=top_block, options=None):
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()
    tb.start()
    tb.show()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()
        qapp.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    qapp.exec_()


if __name__ == '__main__':
    main()


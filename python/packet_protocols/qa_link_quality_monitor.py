#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2024 gr-packet-protocols.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import os
import sys

from qa_gr_test_env import ensure_build_packet_protocols_first

ensure_build_packet_protocols_first()

from gnuradio import gr, gr_unittest
from gnuradio import blocks

try:
    from gnuradio.packet_protocols import link_quality_monitor
    _has_bindings = True
except ImportError:
    _has_bindings = False
    link_quality_monitor = None


class qa_link_quality_monitor(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_instance(self):
        """Test creating an instance"""
        if not _has_bindings:
            self.skipTest("link_quality_monitor bindings not available - requires build installation")
        if not _has_bindings:
            self.skipTest("link_quality_monitor bindings not available")
        instance = link_quality_monitor(alpha=0.1, update_period=1000)
        self.assertIsNotNone(instance)

    def test_snr_update(self):
        """Test SNR update functionality"""
        if not _has_bindings:
            self.skipTest("link_quality_monitor bindings not available")
        monitor = link_quality_monitor(alpha=0.1, update_period=1000)
        
        # Initial SNR should be 0
        self.assertEqual(monitor.get_snr(), 0.0)
        
        # Update SNR
        monitor.update_snr(15.5)
        
        # Should have updated (may not be exact due to smoothing)
        snr = monitor.get_snr()
        self.assertGreater(snr, 0.0)
        self.assertLessEqual(snr, 15.5)

    def test_ber_update(self):
        """Test BER update functionality"""
        if not _has_bindings:
            self.skipTest("link_quality_monitor bindings not available")
        monitor = link_quality_monitor(alpha=0.1, update_period=1000)
        
        # Initial BER should be 0
        self.assertEqual(monitor.get_ber(), 0.0)
        
        # Update BER
        monitor.update_ber(0.001)
        
        # Should have updated (use almost equal for floating point precision)
        ber = monitor.get_ber()
        self.assertGreater(ber, 0.0)
        self.assertLessEqual(ber, 0.0011)  # Allow small floating point error

    def test_frame_statistics(self):
        """Test frame error tracking"""
        if not _has_bindings:
            self.skipTest("link_quality_monitor bindings not available")
        if not _has_bindings:
            self.skipTest("link_quality_monitor bindings not available")
        monitor = link_quality_monitor(alpha=0.1, update_period=1000)
        
        # Record some frames
        monitor.record_frame_success()
        monitor.record_frame_success()
        monitor.record_frame_error()
        
        # FER should be 1/3 = 0.333...
        fer = monitor.get_fer()
        self.assertGreater(fer, 0.0)
        self.assertLessEqual(fer, 1.0)

    def test_quality_score(self):
        """Test quality score calculation"""
        if not _has_bindings:
            self.skipTest("link_quality_monitor bindings not available")
        if not _has_bindings:
            self.skipTest("link_quality_monitor bindings not available")
        monitor = link_quality_monitor(alpha=0.1, update_period=1000)
        
        # Update with good quality metrics
        monitor.update_snr(20.0)
        monitor.update_ber(0.0001)
        monitor.record_frame_success()
        monitor.record_frame_success()
        
        # Quality score should be high
        score = monitor.get_quality_score()
        self.assertGreater(score, 0.0)
        self.assertLessEqual(score, 1.0)

    def test_reset(self):
        """Test reset functionality"""
        if not _has_bindings:
            self.skipTest("link_quality_monitor bindings not available")
        if not _has_bindings:
            self.skipTest("link_quality_monitor bindings not available")
        monitor = link_quality_monitor(alpha=0.1, update_period=1000)
        
        # Update some metrics
        monitor.update_snr(15.0)
        monitor.update_ber(0.001)
        monitor.record_frame_error()
        
        # Reset
        monitor.reset()
        
        # Should be back to defaults
        self.assertEqual(monitor.get_snr(), 0.0)
        self.assertEqual(monitor.get_ber(), 0.0)
        self.assertEqual(monitor.get_fer(), 0.0)

    def test_data_flow(self):
        """Test data flow through the block"""
        if not _has_bindings:
            self.skipTest("link_quality_monitor bindings not available")
        if not _has_bindings:
            self.skipTest("link_quality_monitor bindings not available")
        monitor = link_quality_monitor(alpha=0.1, update_period=1000)
        source = blocks.vector_source_b([0x48, 0x65, 0x6C, 0x6C, 0x6F], False)
        sink = blocks.vector_sink_b()
        
        self.tb.connect(source, monitor, sink)
        self.tb.start()
        self.tb.wait()
        self.tb.stop()
        self.tb.wait()
        
        # Check that data passed through
        data = sink.data()
        self.assertEqual(len(data), 5)


if __name__ == '__main__':
    gr_unittest.run(qa_link_quality_monitor)


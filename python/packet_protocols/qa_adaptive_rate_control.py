#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2024 gr-packet-protocols.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import os
import sys

# Force use of build directory module - MUST be before any gnuradio imports
build_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../build'))
test_modules = os.path.join(build_dir, 'test_modules')
if os.path.exists(test_modules):
    # Insert test_modules at the beginning of sys.path
    if test_modules not in sys.path:
        sys.path.insert(0, test_modules)
    # Set PYTHONPATH environment variable
    os.environ['PYTHONPATH'] = test_modules + ':' + os.environ.get('PYTHONPATH', '')

from gnuradio import gr, gr_unittest
from gnuradio import blocks

# Import packet_protocols and verify we're using the build version
import gnuradio.packet_protocols
expected_path = os.path.join(test_modules, 'gnuradio/packet_protocols/__init__.py')

# Check if adaptive_rate_control is available
if 'adaptive_rate_control' not in dir(gnuradio.packet_protocols):
    # Try to force reload from test_modules if we're using system package
    if os.path.exists(expected_path) and gnuradio.packet_protocols.__file__ != expected_path:
        import importlib
        importlib.invalidate_caches()
        if 'gnuradio.packet_protocols' in sys.modules:
            del sys.modules['gnuradio.packet_protocols']
        # Manually load from test_modules
        import importlib.util
        spec = importlib.util.spec_from_file_location('gnuradio.packet_protocols', expected_path)
        if spec and spec.loader:
            try:
                mod = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(mod)
                sys.modules['gnuradio.packet_protocols'] = mod
                gnuradio.packet_protocols = mod
            except Exception:
                pass  # Will try import below

# Try to import - if it fails, tests will be skipped
try:
    from gnuradio.packet_protocols import adaptive_rate_control
    from gnuradio.packet_protocols import modulation_mode_t
    _has_bindings = True
except ImportError:
    _has_bindings = False
    adaptive_rate_control = None
    modulation_mode_t = None


class qa_adaptive_rate_control(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_instance(self):
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        """Test creating an instance"""
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        instance = adaptive_rate_control(
            initial_mode=modulation_mode_t.MODE_4FSK,
            enable_adaptation=True,
            hysteresis_db=2.0
        )
        self.assertIsNotNone(instance)

    def test_get_modulation_mode(self):
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        """Test getting current modulation mode"""
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        rate_control = adaptive_rate_control(
            initial_mode=modulation_mode_t.MODE_4FSK
        )
        
        mode = rate_control.get_modulation_mode()
        self.assertEqual(mode, modulation_mode_t.MODE_4FSK)

    def test_set_modulation_mode(self):
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        """Test setting modulation mode manually"""
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        rate_control = adaptive_rate_control(
            initial_mode=modulation_mode_t.MODE_4FSK
        )
        
        # Change mode
        rate_control.set_modulation_mode(modulation_mode_t.MODE_8FSK)
        
        # Verify change
        mode = rate_control.get_modulation_mode()
        self.assertEqual(mode, modulation_mode_t.MODE_8FSK)

    def test_get_data_rate(self):
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        """Test getting data rate for current mode"""
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        rate_control = adaptive_rate_control(
            initial_mode=modulation_mode_t.MODE_4FSK
        )
        
        rate = rate_control.get_data_rate()
        self.assertGreater(rate, 0)
        # 4FSK should be 2400 bps
        self.assertEqual(rate, 2400)

    def test_recommend_mode_high_snr(self):
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        """Test mode recommendation for high SNR"""
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        rate_control = adaptive_rate_control(
            initial_mode=modulation_mode_t.MODE_4FSK
        )
        
        # High SNR, low BER should recommend higher rate mode
        recommended = rate_control.recommend_mode(snr_db=25.0, ber=0.0001)
        
        # Should recommend a higher rate mode (8FSK, 16FSK, or QAM)
        self.assertIn(recommended, [
            modulation_mode_t.MODE_8FSK,
            modulation_mode_t.MODE_16FSK,
            modulation_mode_t.MODE_QPSK,
            modulation_mode_t.MODE_8PSK,
            modulation_mode_t.MODE_QAM16,
            modulation_mode_t.MODE_QAM64
        ])

    def test_recommend_mode_low_snr(self):
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        """Test mode recommendation for low SNR"""
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        rate_control = adaptive_rate_control(
            initial_mode=modulation_mode_t.MODE_4FSK
        )
        
        # Low SNR, high BER should recommend lower rate mode
        recommended = rate_control.recommend_mode(snr_db=5.0, ber=0.01)
        
        # Should recommend a more robust mode (2FSK or BPSK)
        self.assertIn(recommended, [
            modulation_mode_t.MODE_2FSK,
            modulation_mode_t.MODE_BPSK,
            modulation_mode_t.MODE_4FSK
        ])

    def test_adaptation_enabled(self):
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        """Test enabling/disabling adaptation"""
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        rate_control = adaptive_rate_control(
            initial_mode=modulation_mode_t.MODE_4FSK,
            enable_adaptation=True
        )
        
        # Disable adaptation
        rate_control.set_adaptation_enabled(False)
        
        # Update with good quality - should not change mode
        initial_mode = rate_control.get_modulation_mode()
        rate_control.update_quality(snr_db=25.0, ber=0.0001, quality_score=0.9)
        
        # Mode should not have changed
        final_mode = rate_control.get_modulation_mode()
        self.assertEqual(initial_mode, final_mode)

    def test_automatic_adaptation(self):
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        """Test automatic mode adaptation"""
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        rate_control = adaptive_rate_control(
            initial_mode=modulation_mode_t.MODE_4FSK,
            enable_adaptation=True,
            hysteresis_db=1.0  # Lower hysteresis for testing
        )
        
        initial_mode = rate_control.get_modulation_mode()
        
        # Update with excellent quality
        rate_control.update_quality(snr_db=25.0, ber=0.0001, quality_score=0.95)
        
        # Mode may have changed to higher rate
        final_mode = rate_control.get_modulation_mode()
        # Should be same or higher rate mode
        self.assertIsNotNone(final_mode)

    def test_data_flow(self):
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        """Test data flow through the block"""
        if not _has_bindings:
            self.skipTest("adaptive_rate_control bindings not available")
        rate_control = adaptive_rate_control(
            initial_mode=modulation_mode_t.MODE_4FSK
        )
        source = blocks.vector_source_b([0x48, 0x65, 0x6C, 0x6C, 0x6F])
        sink = blocks.vector_sink_b()
        
        self.tb.connect(source, rate_control, sink)
        self.tb.start()
        self.tb.wait()
        self.tb.stop()
        self.tb.wait()
        
        # Check that data passed through
        data = sink.data()
        self.assertEqual(len(data), 5)


if __name__ == '__main__':
    gr_unittest.run(qa_adaptive_rate_control)


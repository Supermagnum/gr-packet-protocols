#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright 2025 gr-packet-protocols.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from qa_gr_test_env import ensure_build_packet_protocols_first

ensure_build_packet_protocols_first()

from gnuradio import gr, gr_unittest
from gnuradio.packet_protocols import kiss_tnc

class qa_kiss_tnc(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_instance(self):
        # Test will fail until you pass sensible arguments to the constructor
        # Use /dev/null for testing since we don't have a real serial port
        try:
            instance = kiss_tnc('/dev/null', 9600, False)
        except RuntimeError:
            # Expected if /dev/null can't be opened as serial port
            pass

    def test_001_descriptive_test_name(self):
        # set up fg
        self.tb.run()
        # check data


if __name__ == '__main__':
    gr_unittest.run(qa_kiss_tnc)

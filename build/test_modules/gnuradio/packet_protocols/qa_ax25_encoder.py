#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2025 gr-packet-protocols.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from gnuradio import gr, gr_unittest
# from gnuradio import blocks
try:
    from gnuradio.packet_protocols import ax25_encoder
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.packet_protocols import ax25_encoder

class qa_ax25_encoder(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_instance(self):
        # Test will fail until you pass sensible arguments to the constructor
        instance = ax25_encoder('N0CALL', '0', 'N1CALL', '0')

    def test_001_descriptive_test_name(self):
        # set up fg
        self.tb.run()
        # check data


if __name__ == '__main__':
    gr_unittest.run(qa_ax25_encoder)

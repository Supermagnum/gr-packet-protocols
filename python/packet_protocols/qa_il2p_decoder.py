#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import os
import sys

from qa_gr_test_env import ensure_build_packet_protocols_first

ensure_build_packet_protocols_first()

from gnuradio import blocks, gr, gr_unittest

from gnuradio.packet_protocols import il2p_decoder, il2p_encoder


class qa_il2p_decoder(gr_unittest.TestCase):
    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_random_bits_no_crash(self):
        bits = [((i * 19) >> 4) & 1 for i in range(8000)]
        dec = il2p_decoder()
        src = blocks.vector_source_b(bits, False)
        sink = blocks.vector_sink_b()
        self.tb.connect(src, dec)
        self.tb.connect(dec, sink)
        self.tb.run()
        self.assertIsNotNone(sink.data())

    def test_roundtrip_encoder_decoder(self):
        val = 0x3C
        tb = gr.top_block()
        enc = il2p_encoder("N0CALL", "0", "N1CALL", "0", fec_type=1, add_checksum=True)
        src = blocks.vector_source_b([val], False)
        sink_enc = blocks.vector_sink_b()
        tb.connect(src, enc)
        tb.connect(enc, sink_enc)
        tb.run()
        bits = [int(x) & 1 for x in sink_enc.data()]

        tb2 = gr.top_block()
        dec = il2p_decoder()
        src2 = blocks.vector_source_b(bits, False)
        sink2 = blocks.vector_sink_b()
        tb2.connect(src2, dec)
        tb2.connect(dec, sink2)
        tb2.run()
        raw = bytes([x & 0xFF for x in sink2.data()])
        self.assertGreater(len(raw), 0)
        self.assertEqual(raw[0], val)


if __name__ == "__main__":
    gr_unittest.run(qa_il2p_decoder)

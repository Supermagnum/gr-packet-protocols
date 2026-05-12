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

from gnuradio.packet_protocols import fx25_decoder, fx25_encoder

from qa_codec_utils import fx25_first_payload_byte


class qa_fx25_decoder(gr_unittest.TestCase):
    FEC = 2

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def _encoder_bits(self, byte_val):
        tb = gr.top_block()
        enc = fx25_encoder(fec_type=self.FEC, interleaver_depth=1, add_checksum=True)
        src = blocks.vector_source_b([byte_val], False)
        sink = blocks.vector_sink_b()
        tb.connect(src, enc)
        tb.connect(enc, sink)
        tb.run()
        return [int(x) & 1 for x in sink.data()]

    def test_decoder_from_encoder_bits(self):
        val = 0x3C
        bits = self._encoder_bits(val)
        self.tb = gr.top_block()
        dec = fx25_decoder()
        src = blocks.vector_source_b(bits, False)
        sink = blocks.vector_sink_b()
        self.tb.connect(src, dec)
        self.tb.connect(dec, sink)
        self.tb.run()
        raw = bytes([x & 0xFF for x in sink.data()])
        self.assertGreater(len(raw), 0)
        self.assertEqual(raw[0], val)

    def test_noise_inputs_no_crash(self):
        bits = [((i * 17) >> 3) & 1 for i in range(4000)]
        self.tb = gr.top_block()
        dec = fx25_decoder()
        src = blocks.vector_source_b(bits, False)
        sink = blocks.vector_sink_b()
        self.tb.connect(src, dec)
        self.tb.connect(dec, sink)
        self.tb.run()
        self.assertIsNotNone(sink.data())


if __name__ == "__main__":
    gr_unittest.run(qa_fx25_decoder)

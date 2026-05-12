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

from gnuradio.packet_protocols import ax25_decoder, ax25_encoder

from qa_codec_utils import ax25_ui_payload


class qa_ax25_decoder(gr_unittest.TestCase):
    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def _bits_from_encoder(self, payload_bytes):
        tb = gr.top_block()
        enc = ax25_encoder("N0CALL", "0", "N1CALL", "0")
        src = blocks.vector_source_b(list(payload_bytes), False)
        sink = blocks.vector_sink_b()
        tb.connect(src, enc)
        tb.connect(enc, sink)
        tb.run()
        return [int(x) & 1 for x in sink.data()]

    def test_decoder_follows_encoder_bits(self):
        payload = bytes([0x17])
        bits = self._bits_from_encoder(payload)
        self.assertGreater(len(bits), 0)

        dec = ax25_decoder()
        self.tb = gr.top_block()
        src = blocks.vector_source_b(bits, False)
        sink = blocks.vector_sink_b()
        self.tb.connect(src, dec)
        self.tb.connect(dec, sink)
        self.tb.run()
        raw = bytes([x & 0xFF for x in sink.data()])
        self.assertEqual(ax25_ui_payload(raw), payload)

    def test_reject_random_noise_no_crash(self):
        rng_bits = [((i * 31) >> 5) & 1 for i in range(500)]
        dec = ax25_decoder()
        self.tb = gr.top_block()
        src = blocks.vector_source_b(rng_bits, False)
        sink = blocks.vector_sink_b()
        self.tb.connect(src, dec)
        self.tb.connect(dec, sink)
        self.tb.run()
        self.assertIsNotNone(sink.data())


if __name__ == "__main__":
    gr_unittest.run(qa_ax25_decoder)

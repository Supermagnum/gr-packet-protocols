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

from qa_codec_utils import fx25_first_payload_byte, split_fixed_chunks


class qa_fx25_encoder(gr_unittest.TestCase):
    FEC = 2  # FX25_FEC_RS_255_223 per common.h binding default

    def _run_roundtrip(self, payload_bytes, fec_type=None):
        tb = gr.top_block()
        fec = self.FEC if fec_type is None else fec_type
        enc = fx25_encoder(fec_type=fec, interleaver_depth=1, add_checksum=True)
        dec = fx25_decoder()
        src = blocks.vector_source_b(list(payload_bytes), False)
        sink = blocks.vector_sink_b()
        tb.connect(src, enc)
        tb.connect(enc, dec)
        tb.connect(dec, sink)
        tb.run()
        return bytes([x & 0xFF for x in sink.data()])

    def _k_for_fec(self, fec_type):
        return {
            1: 239,
            2: 223,
            3: 191,
            4: 159,
            5: 127,
            6: 95,
            7: 63,
            8: 31,
        }[fec_type]

    def test_roundtrip_short(self):
        payload = bytes([0x55])
        raw = self._run_roundtrip(payload)
        k = self._k_for_fec(self.FEC)
        self.assertEqual(len(raw), k)
        self.assertEqual(raw[0], payload[0])
        self.assertEqual(raw[1:], bytes(k - 1))

    def test_roundtrip_zeros(self):
        payload = bytes([0x00])
        raw = self._run_roundtrip(payload)
        k = self._k_for_fec(self.FEC)
        self.assertEqual(len(raw), k)
        self.assertEqual(raw[0], 0)
        self.assertEqual(raw[1:], bytes(k - 1))

    def test_roundtrip_sweep_frames(self):
        """Multiple frames: decoder output is concatenated RS data blocks."""
        payload = bytes(range(64))
        raw = self._run_roundtrip(payload)
        k = self._k_for_fec(self.FEC)
        self.assertEqual(len(raw), k * len(payload))
        blocks_out = split_fixed_chunks(raw, k)
        self.assertEqual(len(blocks_out), len(payload))
        for i, blk in enumerate(blocks_out):
            self.assertEqual(fx25_first_payload_byte(blk), payload[i])
            self.assertEqual(blk[1:], bytes(k - 1))

    def test_roundtrip_each_fx25_fec_mode(self):
        for fec_id in range(1, 9):
            with self.subTest(fec_type=fec_id):
                payload = bytes([0xA0 + fec_id])
                raw = self._run_roundtrip(payload, fec_type=fec_id)
                k = self._k_for_fec(fec_id)
                self.assertEqual(len(raw), k)
                self.assertEqual(raw[0], payload[0])


if __name__ == "__main__":
    gr_unittest.run(qa_fx25_encoder)

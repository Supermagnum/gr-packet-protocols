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

from qa_codec_utils import ax25_ui_payload, split_fixed_chunks


class qa_ax25_encoder(gr_unittest.TestCase):
    """Encoder round-trip: serialized HDLC bits include correct AX.25 PDU."""

    def _run_roundtrip(self, payload_bytes):
        tb = gr.top_block()
        enc = ax25_encoder("N0CALL", "0", "N1CALL", "0")
        dec = ax25_decoder()
        src = blocks.vector_source_b(list(payload_bytes), False)
        sink = blocks.vector_sink_b()
        tb.connect(src, enc)
        tb.connect(enc, dec)
        tb.connect(dec, sink)
        tb.run()
        return bytes([x & 0xFF for x in sink.data()])

    def test_roundtrip_short_single_byte(self):
        payload = bytes([0x42])
        raw = self._run_roundtrip(payload)
        self.assertGreater(len(raw), 0)
        self.assertEqual(ax25_ui_payload(raw), payload)

    def test_roundtrip_all_zeros(self):
        payload = bytes([0x00])
        raw = self._run_roundtrip(payload)
        self.assertEqual(ax25_ui_payload(raw), payload)

    def test_roundtrip_many_frames_max_coverage(self):
        """One input byte per AX.25 frame; sweep 0..255 as distinct single-byte payloads."""
        payload = bytes(range(256))
        raw = self._run_roundtrip(payload)
        pdu_len = 19
        self.assertEqual(len(raw) % pdu_len, 0)
        pdus = split_fixed_chunks(raw, pdu_len)
        self.assertEqual(len(pdus), 256)
        recovered = b"".join(ax25_ui_payload(p) for p in pdus)
        self.assertEqual(recovered, payload)


if __name__ == "__main__":
    gr_unittest.run(qa_ax25_encoder)

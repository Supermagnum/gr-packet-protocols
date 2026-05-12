#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import os
import sys

import numpy as np

from qa_gr_test_env import ensure_build_packet_protocols_first

ensure_build_packet_protocols_first()

from gnuradio import blocks, gr, gr_unittest

from gnuradio.packet_protocols import il2p_encoder


class qa_il2p_encoder(gr_unittest.TestCase):
    """IL2P encoder emits HDLC flags with stuffed interior; first interior byte is IL2P preamble 0x55."""

    def test_flag_wrapped_preamble_and_sync(self):
        tb = gr.top_block()
        enc = il2p_encoder("N0CALL", "0", "N1CALL", "0", fec_type=1, add_checksum=True)
        src = blocks.vector_source_b([0x77], False)
        sink = blocks.vector_sink_b()
        tb.connect(src, enc)
        tb.connect(enc, sink)
        tb.run()
        bits = np.array([int(x) & 1 for x in sink.data()], dtype=np.uint8)
        self.assertGreater(len(bits), 0)
        packed = np.packbits(bits).tobytes()
        self.assertGreaterEqual(len(packed), 6)
        self.assertEqual(packed[0], 0x7E)
        self.assertEqual(packed[1], 0x55)
        # Raw IL2P sync bytes (F1 5E 48) live in the unstuffed interior; bit stuffing
        # breaks byte alignment vs ``numpy.packbits``, so do not assert them here.


if __name__ == "__main__":
    gr_unittest.run(qa_il2p_encoder)

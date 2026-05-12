#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Reed-Solomon stress tests aligned with include/gnuradio/packet_protocols/common.h mappings.

Requires the optional third-party package ``reedsolo`` (GF(256) RS). Install in a venv, for example::

    python3 -m venv .venv && .venv/bin/pip install reedsolo

If the package is missing, the suite skips cleanly so CMake/CTest still passes on minimal hosts.
"""

import unittest

try:
    from reedsolo import RSCodec
    from reedsolo import ReedSolomonError
    HAVE_REEDSOLO = True
except ImportError:
    RSCodec = None
    HAVE_REEDSOLO = False

    class ReedSolomonError(Exception):
        """Placeholder when reedsolo is not installed."""

        pass


FX25_PARAMS = {
    1: (255, 239),
    2: (255, 223),
    3: (255, 191),
    4: (255, 159),
    5: (255, 127),
    6: (255, 95),
    7: (255, 63),
    8: (255, 31),
}

IL2P_PARAMS = {
    1: (255, 223),
    2: (255, 239),
    3: (255, 247),
}


def _rs_codec(n: int, k: int):
    if RSCodec is None:
        return None
    nsym = n - k
    return RSCodec(nsym=nsym, nsize=n)


def _corrupt_codeword(cw: bytearray, positions):
    out = bytearray(cw)
    for pos in positions:
        out[pos] ^= 0x5A
    return bytes(out)


@unittest.skipUnless(HAVE_REEDSOLO, "reedsolo not installed (optional dependency)")
class qa_rs_fec_codec(unittest.TestCase):
    def _exercise(self, n: int, k: int, fec_label: str):
        codec = _rs_codec(n, k)
        self.assertIsNotNone(codec)
        t = (n - k) // 2
        msg = bytes([(i * 31 + 7) & 0xFF for i in range(k)])
        cw = codec.encode(msg)
        self.assertEqual(len(cw), n)

        good_positions = list(range(t))
        cw_fix = _corrupt_codeword(bytearray(cw), good_positions)
        dec_ok, _, _ = codec.decode(cw_fix)
        self.assertEqual(dec_ok, msg)

        bad_positions = list(range(t + 1))
        cw_bad = _corrupt_codeword(bytearray(cw), bad_positions)
        with self.assertRaises(ReedSolomonError):
            codec.decode(cw_bad)

    def test_fx25_all_fec_ids(self):
        for fid, (n, k) in FX25_PARAMS.items():
            with self.subTest(fec_id=fid):
                self._exercise(n, k, f"FX.25 fec_type={fid}")

    def test_il2p_all_fec_ids(self):
        for fid, (n, k) in IL2P_PARAMS.items():
            with self.subTest(fec_id=fid):
                self._exercise(n, k, f"IL2P fec_type={fid}")


if __name__ == "__main__":
    unittest.main()

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Lock ``test_rs_golden_vectors`` codewords for the fixed C++ ReedSolomonEncoder.

Why ``reedsolo.RSCodec.encode`` is **not** byte-identical to this codec (even with tuned parameters):

1. **Primitive polynomial** — Both use GF(:math:`2^8`) with the same primitive poly **0x11D**
   (``reedsolo`` default ``prim=0x11D`` matches).

2. **Generator roots (narrow-sense)** — This codec builds :math:`g(x)=\\prod_{j=1}^{2t}(x-\\alpha^j)`
   with :math:`\\alpha=2`. In ``reedsolo``, ``rs_generator_poly(nsym, fcr, generator=2)`` uses roots
   :math:`\\alpha^{\\text{fcr}+i}` for :math:`i=0\\ldots \\text{nsym}-1`. So **fcr=1** aligns the root
   **set** with this codec; the stock ``RSCodec(..., fcr=0)`` default uses roots starting at
   :math:`\\alpha^0`, which is already a different code.

3. **Systematic parity computation (main mismatch)** — ``reedsolo.rs_encode_msg`` implements systematic
   encoding by extended synthetic division anchored at **low** message indices (coefficients
   :math:`x^0\\ldots x^{k-1}`), XORing parity into the trailing ``nsym`` slots of that polynomial view.
   This codec reduces a dividend with message parked at **high** indices (:math:`x^{n-k}\\ldots`),
   forms ``internal = temp \\oplus rem``, then permutes to wire ``[ message | parity ]``. That yields
   **different parity bytes** than ``rs_encode_msg`` even when ``fcr=1`` and ``prim`` match; **wire
   byte order is not a simple reversal** (parity slices still disagree).

4. **Empirical check** — Scanning ``fcr`` in ``0\\ldots31`` does not reproduce the C++ codeword for the
   golden probe message at ``k=239``; message halves align by construction, parity differs.

Regression coverage is therefore **self-contained**: SHA-256 fingerprints per ``k`` for the probe
message ``bytes((i * 31 + 7) & 0xFF for i in range(k))``. Regenerate ``RS_GOLDEN_SHA256`` when the
encoder changes on purpose (see comment below).

Decoder behaviour on corrupted inputs is covered elsewhere (C++ flip tests, Python FEC QA).
"""

from __future__ import annotations

import hashlib
import os
import subprocess
import sys
import unittest


def _golden_vectors_exe():
    """Resolve ``test_rs_golden_vectors`` for CTest (CMake ``copy_module_for_tests`` layout)."""
    exe = os.environ.get("RS_CPP_GOLDEN_EXE")
    if exe and not exe.startswith("$<") and os.path.isfile(exe):
        return exe
    pypath = os.environ.get("PYTHONPATH", "")
    if not pypath:
        return None
    head = pypath.split(os.pathsep)[0].strip()
    if not head:
        return None
    candidate = os.path.abspath(os.path.join(head, "..", "lib", "test_rs_golden_vectors"))
    if os.path.isfile(candidate):
        return candidate
    return None


# SHA-256 of each 255-byte codeword from test_rs_golden_vectors (message probe above).
# Regenerate after intentional RS encoder changes:
#
#   ./build/lib/test_rs_golden_vectors | python3 -c '
#   import sys, hashlib
#   for line in sys.stdin:
#       line = line.strip()
#       if not line.startswith("ENC "): continue
#       rest = line[4:].strip().split(None, 1)
#       k = int(rest[0][2:])
#       b = bytes.fromhex(rest[1])
#       print("%d: %s" % (k, hashlib.sha256(b).hexdigest()))
#   '
RS_GOLDEN_SHA256 = {
    239: "3547b7860d1d93ede3a7a46f3d3cc9d1c0d54cefd34c46ff25e17b38c9f380e6",
    223: "8de428f6586d4a956b945c8f78fbe0e483ed0dc310b8419ec81cce12b8591ffb",
    191: "ab12726346bf9d9bf030b661137c756f42cd5f655c2d5c24d80120d104245971",
    159: "5b48a1cd4750119776eecc81a570a56198b98b00b6025d24a7435ec2f2eb8d3d",
    127: "96d44e30216c5dab9cb36490d7830ff8d83988dd2c61c2a515343667028c9936",
    95: "b9709e7905fc96a7f64ce64478c7b8d67f1bce2738c5714a78f3c9055e10e3c1",
    63: "e458c1ed845cce88b869c9db545fb11a3e80a198df313c8f09b82bb283754011",
    31: "1c0d6455853e17018d9c1e3c02d03d818839fc055ac8f8f6c99cf2d12db66eb7",
    247: "2a565bc737c80c439d40936d9641d2115a5d4cc4c59637fabdb220de4298219d",
}


def _parse_enc_lines(stdout: str):
    out = {}
    for line in stdout.splitlines():
        line = line.strip()
        if not line.startswith("ENC "):
            continue
        rest = line[4:].strip()
        parts = rest.split(None, 1)
        if len(parts) != 2 or not parts[0].startswith("k="):
            continue
        k = int(parts[0][2:])
        hexbytes = parts[1].strip()
        out[k] = bytes.fromhex(hexbytes)
    return out


class qa_rs_cpp_golden_match(unittest.TestCase):
    """Uses ``RS_CPP_GOLDEN_EXE`` or infers ``test_rs_golden_vectors`` next to the CMake build tree."""

    def test_harness_exit_success(self):
        exe = _golden_vectors_exe()
        self.assertIsNotNone(exe, "RS_CPP_GOLDEN_EXE or inferred build path must name test_rs_golden_vectors")
        self.assertTrue(os.path.isfile(exe), exe)
        proc = subprocess.run([exe], capture_output=True, text=True, check=False)
        self.assertEqual(proc.returncode, 0, proc.stderr)

    def test_codeword_sha256_matches_locked_table(self):
        exe = _golden_vectors_exe()
        self.assertIsNotNone(exe, "RS_CPP_GOLDEN_EXE or inferred build path must name test_rs_golden_vectors")
        self.assertTrue(os.path.isfile(exe), exe)
        proc = subprocess.run([exe], capture_output=True, text=True, check=False)
        self.assertEqual(proc.returncode, 0, proc.stderr)
        enc_map = _parse_enc_lines(proc.stdout)
        ks = sorted(enc_map.keys())
        self.assertEqual(
            ks,
            sorted(RS_GOLDEN_SHA256.keys()),
            "unexpected k set from C++ harness",
        )
        for k in ks:
            cw = enc_map[k]
            self.assertEqual(len(cw), 255, k)
            digest = hashlib.sha256(cw).hexdigest()
            self.assertEqual(
                digest,
                RS_GOLDEN_SHA256[k],
                "codeword changed for k=%s; update RS_GOLDEN_SHA256 if intentional" % k,
            )


if __name__ == "__main__":
    if len(sys.argv) > 1:
        os.environ["RS_CPP_GOLDEN_EXE"] = sys.argv[1]
        unittest.main(argv=[sys.argv[0]])
    else:
        unittest.main()

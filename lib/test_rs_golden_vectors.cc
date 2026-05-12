// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Emit Reed-Solomon codewords from the C++ implementation for golden-vector checks
 * (see python/packet_protocols/qa_rs_cpp_golden_match.py).
 *
 * Why not ``reedsolo`` byte-for-byte: same GF(2^8) primitive polynomial 0x11D as ``reedsolo`` default,
 * and generator roots α^1 … α^{2t} match ``RSCodec(..., fcr=1, prim=0x11D, generator=2)`` in principle,
 * but ``reedsolo.rs_encode_msg`` fixes parity via extended synthetic division with the message tied to
 * low-degree coefficients. This codec uses a parity-low / high-degree-message reduction (temp ⊕ rem),
 * then wire order [message | parity]. That coefficient-domain systematic mapping differs from
 * ``rs_encode_msg``, so parity bytes differ even when fcr is aligned; stock ``fcr=0`` also shifts the
 * generator root set (roots α^0 …). SHA-256 locks in this encoder output (qa_rs_cpp_golden_match.py).
 */

#include <gnuradio/packet_protocols/common.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

void emit_one(int k) {
    ReedSolomonEncoder enc(255, k);
    ReedSolomonDecoder dec(255, k);
    std::vector<uint8_t> msg(static_cast<size_t>(k));
    for (int i = 0; i < k; ++i) {
        msg[static_cast<size_t>(i)] = static_cast<uint8_t>((i * 31 + 7) & 0xFF);
    }
    std::vector<uint8_t> cw = enc.encode(msg);
    if (static_cast<int>(cw.size()) != 255) {
        std::fprintf(stderr, "encode length mismatch k=%d\n", k);
        std::exit(1);
    }
    std::vector<uint8_t> recovered;
    if (!dec.decode(cw, recovered) || recovered != msg) {
        std::fprintf(stderr, "C++ RS encode-decode round-trip failed k=%d\n", k);
        std::exit(1);
    }
    std::printf("ENC k=%d ", k);
    for (uint8_t b : cw) {
        std::printf("%02x", static_cast<unsigned>(b));
    }
    std::printf("\n");
}

} // namespace

int main() {
    /* Union of k values used by FX.25 and IL2P FEC types in common.h */
    const int ks[] = { 239, 223, 191, 159, 127, 95, 63, 31, 247 };
    for (int k : ks) {
        emit_one(k);
    }
    return 0;
}

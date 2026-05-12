// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Single-byte flips in the data region (wire indices 0..k-1) and parity region (k..254) of each
 * RS(255,k) codeword; decoder must recover the original payload (wire order: message | parity).
 */

#include <gnuradio/packet_protocols/common.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

void check_data_flip(int k) {
    ReedSolomonEncoder enc(255, k);
    ReedSolomonDecoder dec(255, k);
    std::vector<uint8_t> msg(static_cast<size_t>(k));
    for (int i = 0; i < k; ++i) {
        msg[static_cast<size_t>(i)] = static_cast<uint8_t>((i * 47 + 11) & 0xFF);
    }
    std::vector<uint8_t> cw = enc.encode(msg);
    cw[0] = static_cast<uint8_t>(cw[0] ^ 0x5AU);
    std::vector<uint8_t> out;
    if (!dec.decode(cw, out)) {
        std::fprintf(stderr, "RS decode failed after data-region flip k=%d\n", k);
        std::exit(1);
    }
    if (out != msg) {
        std::fprintf(stderr, "RS decode wrong payload after data-region flip k=%d\n", k);
        std::exit(1);
    }
}

void check_parity_flip(int k, int wire_index) {
    ReedSolomonEncoder enc(255, k);
    ReedSolomonDecoder dec(255, k);
    std::vector<uint8_t> msg(static_cast<size_t>(k));
    for (int i = 0; i < k; ++i) {
        msg[static_cast<size_t>(i)] = static_cast<uint8_t>((i * 47 + 11) & 0xFF);
    }
    std::vector<uint8_t> cw = enc.encode(msg);
    if (wire_index < k || wire_index >= 255) {
        std::fprintf(stderr, "internal test error: bad wire_index=%d k=%d\n", wire_index, k);
        std::exit(1);
    }
    cw[static_cast<size_t>(wire_index)] =
        static_cast<uint8_t>(cw[static_cast<size_t>(wire_index)] ^ 0x5AU);
    std::vector<uint8_t> out;
    if (!dec.decode(cw, out)) {
        std::fprintf(stderr, "RS decode failed after parity-region flip k=%d idx=%d\n", k, wire_index);
        std::exit(1);
    }
    if (out != msg) {
        std::fprintf(stderr, "RS decode wrong payload after parity-region flip k=%d idx=%d\n", k,
                     wire_index);
        std::exit(1);
    }
}

} // namespace

int main() {
    const int ks[] = { 239, 223, 191, 159, 127, 95, 63, 31, 247 };
    for (int k : ks) {
        check_data_flip(k);
        check_parity_flip(k, k);
        check_parity_flip(k, (k + 254) / 2);
        check_parity_flip(k, 254);
    }
    return 0;
}

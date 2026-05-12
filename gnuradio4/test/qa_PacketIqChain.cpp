// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/ut.hpp>
#include <gnuradio-4.0/packet_protocols/detail/FxIl2pCodec.hpp>
#include <gnuradio-4.0/packet_protocols/detail/ProtocolConstants.hpp>
#include <gnuradio-4.0/packet_protocols/detail/ReedSolomon.hpp>

#include <vector>

#include <algorithm>

using namespace boost::ut;

using gnuradio4::packet_protocols::detail::decodeFx25Bytes;
using gnuradio4::packet_protocols::detail::encodeFx25Bytes;
using gnuradio4::packet_protocols::detail::fx25_fec_rs255_223;
using gnuradio4::packet_protocols::detail::ReedSolomonDecoder;
using gnuradio4::packet_protocols::detail::ReedSolomonEncoder;

namespace {

static suite PacketIqChainPhyIndependent = [] {
    "RS pristine encode-decode + FX.25 bytes (IQ analogue deferred to GR 3.10 qa_packet_iq_chain.py)"_test = [] {
        const int ks[] = {239, 223, 191};
        for (int k : ks) {
            ReedSolomonEncoder enc(255, k);
            ReedSolomonDecoder dec(255, k);
            std::vector<std::uint8_t> msg(static_cast<std::size_t>(k), 0xA5U);
            auto                        cw = enc.encode(msg);
            std::vector<std::uint8_t>   out;
            expect(dec.decode(cw, out));
            expect(out == msg);
        }

        std::vector<std::uint8_t> inner{0x01U, 0x02U, 0x03U};
        auto                      fr = encodeFx25Bytes(inner, fx25_fec_rs255_223, 1, true);
        expect(fr.has_value());
        auto back = decodeFx25Bytes(*fr, true);
        expect(back.has_value());
        expect(std::equal(inner.begin(), inner.end(), back->begin()));
    };
};

} // namespace

int main() { return boost::ut::cfg<boost::ut::override>.run(); }

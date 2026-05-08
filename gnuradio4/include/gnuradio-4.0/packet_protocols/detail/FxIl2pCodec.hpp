// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_DETAIL_FX_IL2P_CODEC_HPP
#define GNURADIO4_PACKET_PROTOCOLS_DETAIL_FX_IL2P_CODEC_HPP

#include <gnuradio-4.0/packet_protocols/detail/ProtocolConstants.hpp>
#include <gnuradio-4.0/packet_protocols/detail/ReedSolomon.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace gnuradio4::packet_protocols::detail {

namespace {

[[nodiscard]] inline std::unique_ptr<ReedSolomonEncoder> makeFx25RsEncoder(std::uint8_t fec_type) {
    switch (fec_type) {
    case fx25_fec_rs255_239:
        return std::make_unique<ReedSolomonEncoder>(255, 239);
    case fx25_fec_rs255_223:
        return std::make_unique<ReedSolomonEncoder>(255, 223);
    case fx25_fec_rs255_191:
        return std::make_unique<ReedSolomonEncoder>(255, 191);
    case fx25_fec_rs255_159:
        return std::make_unique<ReedSolomonEncoder>(255, 159);
    case fx25_fec_rs255_127:
        return std::make_unique<ReedSolomonEncoder>(255, 127);
    case fx25_fec_rs255_95:
        return std::make_unique<ReedSolomonEncoder>(255, 95);
    case fx25_fec_rs255_63:
        return std::make_unique<ReedSolomonEncoder>(255, 63);
    case fx25_fec_rs255_31:
        return std::make_unique<ReedSolomonEncoder>(255, 31);
    default:
        return std::make_unique<ReedSolomonEncoder>(255, 223);
    }
}

[[nodiscard]] inline std::unique_ptr<ReedSolomonDecoder> makeFx25RsDecoder(std::uint8_t fec_type) {
    switch (fec_type) {
    case fx25_fec_rs255_239:
        return std::make_unique<ReedSolomonDecoder>(255, 239);
    case fx25_fec_rs255_223:
        return std::make_unique<ReedSolomonDecoder>(255, 223);
    case fx25_fec_rs255_191:
        return std::make_unique<ReedSolomonDecoder>(255, 191);
    case fx25_fec_rs255_159:
        return std::make_unique<ReedSolomonDecoder>(255, 159);
    case fx25_fec_rs255_127:
        return std::make_unique<ReedSolomonDecoder>(255, 127);
    case fx25_fec_rs255_95:
        return std::make_unique<ReedSolomonDecoder>(255, 95);
    case fx25_fec_rs255_63:
        return std::make_unique<ReedSolomonDecoder>(255, 63);
    case fx25_fec_rs255_31:
        return std::make_unique<ReedSolomonDecoder>(255, 31);
    default:
        return std::make_unique<ReedSolomonDecoder>(255, 223);
    }
}

[[nodiscard]] inline std::vector<std::uint8_t> fx25ApplyRsEncode(ReedSolomonEncoder& encoder, const std::vector<std::uint8_t>& data) {
    std::vector<std::uint8_t> out;
    const int block_size      = encoder.get_data_length();
    const int total_blocks = static_cast<int>((data.size() + static_cast<std::size_t>(block_size) - 1UZ) / static_cast<std::size_t>(block_size));
    for (int b = 0; b < total_blocks; ++b) {
        std::vector<std::uint8_t> blk(static_cast<std::size_t>(block_size), 0U);
        for (int i = 0; i < block_size; ++i) {
            const int idx = b * block_size + i;
            if (idx < static_cast<int>(data.size())) {
                blk[static_cast<std::size_t>(i)] = data[static_cast<std::size_t>(idx)];
            }
        }
        std::vector<std::uint8_t> coded = encoder.encode(blk);
        out.insert(out.end(), coded.begin(), coded.end());
    }
    return out;
}

[[nodiscard]] inline std::vector<std::uint8_t> fx25Interleave(std::vector<std::uint8_t> data, int depth) noexcept {
    if (depth <= 1 || data.empty()) {
        return data;
    }
    std::vector<std::uint8_t> interleaved(data.size());
    for (std::size_t i = 0; i < data.size(); ++i) {
        std::size_t new_pos =
            static_cast<std::size_t>((static_cast<long long>(i) * static_cast<long long>(depth))
                                     % static_cast<long long>(data.size()));
        interleaved[new_pos] = data[i];
    }
    return interleaved;
}

[[nodiscard]] inline std::vector<std::uint8_t> fx25Deinterleave(const std::vector<std::uint8_t>& data, int depth) noexcept {
    return fx25Interleave(data, depth);
}

[[nodiscard]] inline std::uint16_t fx25Crc16(const std::vector<std::uint8_t>& buf, std::size_t len) noexcept {
    std::uint16_t chk = 0xFFFF;
    for (std::size_t i = 0; i < len && i < buf.size(); ++i) {
        chk ^= buf[i];
        for (int j = 0; j < 8; ++j) {
            if (chk & 0x0001U) {
                chk = static_cast<std::uint16_t>((chk >> 1U) ^ 0x8408U);
            } else {
                chk = static_cast<std::uint16_t>(chk >> 1U);
            }
        }
    }
    return static_cast<std::uint16_t>(chk ^ 0xFFFFU);
}

[[nodiscard]] inline std::unique_ptr<ReedSolomonEncoder> makeIl2pRsEncoder(std::uint8_t fec_type) {
    switch (fec_type) {
    case il2p_fec_rs255_239:
        return std::make_unique<ReedSolomonEncoder>(255, 239);
    case il2p_fec_rs255_247:
        return std::make_unique<ReedSolomonEncoder>(255, 247);
    case il2p_fec_rs255_223:
    default:
        return std::make_unique<ReedSolomonEncoder>(255, 223);
    }
}

[[nodiscard]] inline std::unique_ptr<ReedSolomonDecoder> makeIl2pRsDecoder(std::uint8_t fec_type) {
    switch (fec_type) {
    case il2p_fec_rs255_239:
        return std::make_unique<ReedSolomonDecoder>(255, 239);
    case il2p_fec_rs255_247:
        return std::make_unique<ReedSolomonDecoder>(255, 247);
    case il2p_fec_rs255_223:
    default:
        return std::make_unique<ReedSolomonDecoder>(255, 223);
    }
}

[[nodiscard]] inline std::vector<std::uint8_t> il2pScramble(const std::vector<std::uint8_t>& data) noexcept {
    std::vector<std::uint8_t> scrambled(data.size());
    std::uint8_t scrambler_state = 0x1FU;
    for (std::size_t i = 0; i < data.size(); ++i) {
        std::uint8_t output_byte = 0;
        for (int bit = 0; bit < 8; ++bit) {
            std::uint8_t feedback =
                static_cast<std::uint8_t>(((static_cast<unsigned>(scrambler_state >> 4) ^ static_cast<unsigned>(scrambler_state >> 0)) & 0x01U));
            scrambler_state = static_cast<std::uint8_t>(((scrambler_state << 1) | feedback) & 0x1FU);
            std::uint8_t input_bit = static_cast<std::uint8_t>((static_cast<unsigned>(data[i]) >> static_cast<unsigned>(7 - bit)) & 1U);
            output_byte |= static_cast<std::uint8_t>((static_cast<unsigned>(input_bit ^ feedback) << static_cast<unsigned>(7 - bit)));
        }
        scrambled[i] = output_byte;
    }
    return scrambled;
}

[[nodiscard]] inline std::vector<std::uint8_t>
il2pPayloadRsEncode(ReedSolomonEncoder& enc, const std::vector<std::uint8_t>& payload) noexcept {
    return fx25ApplyRsEncode(enc, payload);
}

[[nodiscard]] inline std::vector<std::uint8_t>
il2pPayloadRsDecode(ReedSolomonDecoder& dec, const std::vector<std::uint8_t>& scrambled_fec_blob) noexcept {
    std::vector<std::uint8_t> decoded;
    const int                          nblock = dec.get_code_length();
    const int                              total_blocks = static_cast<int>(
        (scrambled_fec_blob.size() + static_cast<std::size_t>(nblock) - 1UZ)
        / static_cast<std::size_t>(nblock));
    for (int b = 0; b < total_blocks; ++b) {
        std::vector<std::uint8_t> blk(static_cast<std::size_t>(nblock), 0U);
        for (int i = 0; i < nblock; ++i) {
            const int idx = b * nblock + i;
            if (idx < static_cast<int>(scrambled_fec_blob.size())) {
                blk[static_cast<std::size_t>(i)] = scrambled_fec_blob[static_cast<std::size_t>(idx)];
            }
        }
        std::vector<std::uint8_t> out = dec.decode(blk);
        decoded.insert(decoded.end(), out.begin(), out.end());
    }
    return decoded;
}

[[nodiscard]] inline std::vector<std::uint8_t>
il2pAddAddresses(const std::string& dst_call, const std::string& dst_ssid_str, const std::string& src_call, const std::string& src_ssid_str) {
    std::vector<std::uint8_t> out;
    for (std::size_t i = 0; i < 6UZ; ++i) {
        if (i < dst_call.size()) {
            out.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(dst_call[i]) << 1U));
        } else {
            out.push_back(static_cast<std::uint8_t>(' ' << 1U));
        }
    }
    const int ssid_dest = dst_ssid_str.empty() ? 0 : std::stoi(dst_ssid_str);
    std::uint8_t ssid_byte = static_cast<std::uint8_t>((ssid_dest & 0x0F) << 1U);
    out.push_back(ssid_byte);
    for (std::size_t i = 0; i < 6UZ; ++i) {
        if (i < src_call.size()) {
            out.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(src_call[i]) << 1U));
        } else {
            out.push_back(static_cast<std::uint8_t>(' ' << 1U));
        }
    }
    const int ssid_src = src_ssid_str.empty() ? 0 : std::stoi(src_ssid_str);
    ssid_byte = static_cast<std::uint8_t>(((ssid_src & 0x0F) << 1U) | 1U); // extended last flag
    out.push_back(ssid_byte);
    return out;
}

[[nodiscard]] inline std::uint32_t il2pCrc32(const std::vector<std::uint8_t>& buf, std::size_t len_excl_trailer) noexcept {
    std::uint32_t chk = 0xFFFFFFFFU;
    for (std::size_t i = 0; i < len_excl_trailer && i < buf.size(); ++i) {
        chk ^= buf[i];
        for (int j = 0; j < 8; ++j) {
            if (chk & 0x00000001U) {
                chk = (chk >> 1U) ^ 0xEDB88320U;
            } else {
                chk >>= 1U;
            }
        }
    }
    return chk ^ 0xFFFFFFFFU;
}

} // namespace

[[nodiscard]] inline std::optional<std::vector<std::uint8_t>> encodeFx25Bytes(const std::vector<std::uint8_t>& inner_ax25_octets,
                                                                            std::uint8_t fec_type,
                                                                            std::uint8_t interleaver_depth,
                                                                            bool add_checksum) noexcept {
    auto enc                             = makeFx25RsEncoder(fec_type);
    std::vector<std::uint8_t> fec_encoded = fx25ApplyRsEncode(*enc, inner_ax25_octets);
    std::vector<std::uint8_t> payload     = fx25Interleave(std::move(fec_encoded), interleaver_depth);
    std::vector<std::uint8_t> frame;
    frame.push_back(0x7EU);
    frame.push_back(static_cast<std::uint8_t>('F'));
    frame.push_back(static_cast<std::uint8_t>('X'));
    frame.push_back(static_cast<std::uint8_t>('2'));
    frame.push_back(static_cast<std::uint8_t>('5'));
    frame.push_back(fec_type);
    frame.push_back(interleaver_depth);
    frame.insert(frame.end(), payload.begin(), payload.end());
    if (add_checksum) {
        const std::uint16_t c = fx25Crc16(frame, frame.size());
        frame.push_back(static_cast<std::uint8_t>(c & 0xFFU));
        frame.push_back(static_cast<std::uint8_t>((c >> 8U) & 0xFFU));
    }
    return frame;
}

[[nodiscard]] inline std::optional<std::vector<std::uint8_t>> decodeFx25Bytes(const std::vector<std::uint8_t>& frame,
                                                                              bool had_checksum) noexcept {
    if (frame.size() < 8UZ) {
        return std::nullopt;
    }
    if (frame[0] != 0x7EU || frame[1] != 'F' || frame[2] != 'X' || frame[3] != '2' || frame[4] != '5') {
        return std::nullopt;
    }
    const std::uint8_t fec_type            = frame[5];
    const std::uint8_t interleaver_depth   = frame[6];
    const std::size_t body_end            = had_checksum ? frame.size() - 2UZ : frame.size();
    if (body_end <= 7UZ) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> rs_blob(frame.begin() + 7, frame.begin() + static_cast<std::ptrdiff_t>(body_end));
    if (had_checksum) {
        const std::uint16_t got =
            static_cast<std::uint16_t>(static_cast<unsigned>(frame[frame.size() - 2])
                                       | (static_cast<unsigned>(frame[frame.size() - 1]) << 8U));
        const std::uint16_t want = fx25Crc16(frame, frame.size() - 2UZ);
        if (got != want) {
            return std::nullopt;
        }
    }
    auto deinter            = fx25Deinterleave(std::move(rs_blob), interleaver_depth);
    auto dec                = makeFx25RsDecoder(fec_type);
    std::vector<std::uint8_t> out;
    const int nblock        = dec->get_code_length();
    const int total_blocks = static_cast<int>((deinter.size() + static_cast<std::size_t>(nblock) - 1UZ) / static_cast<std::size_t>(nblock));
    for (int b = 0; b < total_blocks; ++b) {
        std::vector<std::uint8_t> blk(static_cast<std::size_t>(nblock), 0U);
        for (int i = 0; i < nblock; ++i) {
            const int idx = b * nblock + i;
            if (idx < static_cast<int>(deinter.size())) {
                blk[static_cast<std::size_t>(i)] = deinter[static_cast<std::size_t>(idx)];
            }
        }
        std::vector<std::uint8_t> part = dec->decode(blk);
        out.insert(out.end(), part.begin(), part.end());
    }
    return out;
}

[[nodiscard]] inline std::optional<std::vector<std::uint8_t>>
encodeIl2pBytes(const std::vector<std::uint8_t>& payload,
                const std::string& dst_call,
                const std::string& dst_ssid,
                const std::string& src_call,
                const std::string& src_ssid,
                std::uint8_t fec_type,
                bool add_checksum) noexcept {
    try {
        auto enc = makeIl2pRsEncoder(fec_type);
        std::vector<std::uint8_t> fec        = il2pPayloadRsEncode(*enc, payload);
        std::vector<std::uint8_t> scrambled = il2pScramble(fec);
        std::vector<std::uint8_t> frame;
        frame.push_back(il2p_preamble);
        frame.push_back(0xF1U);
        frame.push_back(0x5EU);
        frame.push_back(0x48U);
        frame.push_back(fec_type);
        std::vector<std::uint8_t> addrs = il2pAddAddresses(dst_call, dst_ssid, src_call, src_ssid);
        frame.insert(frame.end(), addrs.begin(), addrs.end());
        frame.insert(frame.end(), scrambled.begin(), scrambled.end());
        if (add_checksum) {
            const std::uint32_t c = il2pCrc32(frame, frame.size());
            frame.push_back(static_cast<std::uint8_t>(c & 0xFFU));
            frame.push_back(static_cast<std::uint8_t>((c >> 8U) & 0xFFU));
            frame.push_back(static_cast<std::uint8_t>((c >> 16U) & 0xFFU));
            frame.push_back(static_cast<std::uint8_t>((c >> 24U) & 0xFFU));
        }
        return frame;
    } catch (...) {
        return std::nullopt;
    }
}

[[nodiscard]] inline std::optional<std::vector<std::uint8_t>> decodeIl2pBytes(const std::vector<std::uint8_t>& frame,
                                                                              bool had_checksum) noexcept {
    if (frame.size() < 20UZ) {
        return std::nullopt;
    }
    if (frame[0] != il2p_preamble || frame[1] != 0xF1U || frame[2] != 0x5EU || frame[3] != 0x48U) {
        return std::nullopt;
    }
    const std::uint8_t fec_type = frame[4];
    constexpr std::size_t header_len = 19UZ;
    if (frame.size() <= header_len) {
        return std::nullopt;
    }
    const std::size_t tail = had_checksum ? 4UZ : 0UZ;
    if (frame.size() < header_len + tail) {
        return std::nullopt;
    }
    const std::size_t body_end = frame.size() - tail;
    if (had_checksum) {
        const std::uint32_t got = static_cast<std::uint32_t>(frame[frame.size() - 4])
            | (static_cast<std::uint32_t>(frame[frame.size() - 3]) << 8U)
            | (static_cast<std::uint32_t>(frame[frame.size() - 2]) << 16U)
            | (static_cast<std::uint32_t>(frame[frame.size() - 1]) << 24U);
        const std::uint32_t want = il2pCrc32(frame, frame.size() - 4UZ);
        if (got != want) {
            return std::nullopt;
        }
    }
    std::vector<std::uint8_t> scrambled(frame.begin() + static_cast<std::ptrdiff_t>(header_len),
                                        frame.begin() + static_cast<std::ptrdiff_t>(body_end));
    auto scrambled_desc = il2pScramble(scrambled);
    auto dec              = makeIl2pRsDecoder(fec_type);
    return il2pPayloadRsDecode(*dec, scrambled_desc);
}

} // namespace gnuradio4::packet_protocols::detail

#endif

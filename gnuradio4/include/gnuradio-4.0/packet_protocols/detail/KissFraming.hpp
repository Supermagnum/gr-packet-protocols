// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_DETAIL_KISS_FRAMING_HPP
#define GNURADIO4_PACKET_PROTOCOLS_DETAIL_KISS_FRAMING_HPP

#include <gnuradio-4.0/packet_protocols/detail/ProtocolConstants.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace gnuradio4::packet_protocols::detail {

inline void kissEscapeByte(std::uint8_t byte, std::vector<std::uint8_t>& out) {
    if (byte == kiss_fend || byte == kiss_fesc) {
        out.push_back(kiss_fesc);
        out.push_back(byte == kiss_fend ? kiss_tfend : kiss_tfesc);
    } else {
        out.push_back(byte);
    }
}

[[nodiscard]] inline std::vector<std::uint8_t>
kissFrame(std::uint8_t command_byte, std::vector<std::uint8_t> payload) noexcept {
    std::vector<std::uint8_t> framed;
    framed.push_back(kiss_fend);
    kissEscapeByte(command_byte, framed);
    for (std::uint8_t u : payload) {
        kissEscapeByte(u, framed);
    }
    framed.push_back(kiss_fend);
    return framed;
}

[[nodiscard]] inline std::optional<std::pair<std::uint8_t, std::vector<std::uint8_t>>>
kissDeframeChunk(const std::vector<std::uint8_t>& in) noexcept {
    if (in.empty()) {
        return std::nullopt;
    }
    if (in.front() != kiss_fend || in.back() != kiss_fend) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> body;
    body.reserve(in.size());
    bool esc = false;
    for (std::size_t i = 1; i + 1 < in.size(); ++i) {
        std::uint8_t u = in[i];
        if (esc) {
            if (u == kiss_tfend) {
                body.push_back(kiss_fend);
            } else if (u == kiss_tfesc) {
                body.push_back(kiss_fesc);
            } else {
                return std::nullopt;
            }
            esc = false;
            continue;
        }
        if (u == kiss_fesc) {
            esc = true;
            continue;
        }
        body.push_back(u);
    }
    if (esc || body.empty()) {
        return std::nullopt;
    }
    std::uint8_t cmd = body.front();
    body.erase(body.begin());
    return std::pair<std::uint8_t, std::vector<std::uint8_t>>{cmd, std::move(body)};
}

} // namespace gnuradio4::packet_protocols::detail

#endif

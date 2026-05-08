// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_DETAIL_NEGOTIATION_FRAME_HPP
#define GNURADIO4_PACKET_PROTOCOLS_DETAIL_NEGOTIATION_FRAME_HPP

#include <gnuradio-4.0/packet_protocols/detail/ModulationMode.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace gnuradio4::packet_protocols::detail {

[[nodiscard]] inline std::vector<std::uint8_t> encode_negotiation_request(
    const std::string& station_id,
    ModulationMode proposed_mode,
    const std::vector<ModulationMode>& supported_modes) {
    std::vector<std::uint8_t> frame;
    const std::uint8_t id_len = static_cast<std::uint8_t>(std::min<std::size_t>(255UZ, station_id.length()));
    frame.push_back(id_len);
    frame.insert(frame.end(), station_id.begin(), station_id.begin() + id_len);
    frame.push_back(static_cast<std::uint8_t>(proposed_mode));
    const std::uint8_t num_modes =
        static_cast<std::uint8_t>(std::min<std::size_t>(8UZ, supported_modes.size()));
    frame.push_back(num_modes);
    for (std::size_t i = 0; i < num_modes; ++i) {
        frame.push_back(static_cast<std::uint8_t>(supported_modes[i]));
    }
    return frame;
}

[[nodiscard]] inline bool decode_negotiation_request(
    const std::uint8_t* data,
    std::size_t length,
    std::string& station_id,
    ModulationMode& proposed_mode,
    std::vector<ModulationMode>& supported_modes) {
    if (length < 3 || data == nullptr) {
        return false;
    }
    std::size_t pos = 0;
    std::uint8_t id_len = data[pos++];
    if (pos + id_len + 2 > length) {
        return false;
    }
    station_id.assign(reinterpret_cast<const char*>(&data[pos]), id_len);
    pos += id_len;
    proposed_mode = static_cast<ModulationMode>(data[pos++]);
    std::uint8_t num_modes = data[pos++];
    if (pos + num_modes > length) {
        return false;
    }
    supported_modes.clear();
    for (std::uint8_t i = 0; i < num_modes; ++i) {
        supported_modes.push_back(static_cast<ModulationMode>(data[pos++]));
    }
    return true;
}

[[nodiscard]] inline std::vector<std::uint8_t>
encode_negotiation_response(const std::string& station_id, bool accepted, ModulationMode negotiated_mode) {
    std::vector<std::uint8_t> frame;
    const std::uint8_t id_len = static_cast<std::uint8_t>(std::min<std::size_t>(255UZ, station_id.length()));
    frame.push_back(id_len);
    frame.insert(frame.end(), station_id.begin(), station_id.begin() + id_len);
    frame.push_back(accepted ? 1 : 0);
    frame.push_back(static_cast<std::uint8_t>(negotiated_mode));
    return frame;
}

[[nodiscard]] inline bool decode_negotiation_response(
    const std::uint8_t* data,
    std::size_t length,
    std::string& station_id,
    bool& accepted,
    ModulationMode& negotiated_mode) {
    if (length < 3 || data == nullptr) {
        return false;
    }
    std::size_t pos             = 0;
    std::uint8_t id_len          = data[pos++];
    if (pos + id_len + 2 > length) {
        return false;
    }
    station_id.assign(reinterpret_cast<const char*>(&data[pos]), id_len);
    pos += id_len;
    accepted        = data[pos++] != 0;
    negotiated_mode = static_cast<ModulationMode>(data[pos++]);
    return true;
}

[[nodiscard]] inline std::vector<std::uint8_t> encode_mode_change(const std::string& station_id, ModulationMode new_mode) {
    std::vector<std::uint8_t> frame;
    const std::uint8_t id_len = static_cast<std::uint8_t>(std::min<std::size_t>(255UZ, station_id.length()));
    frame.push_back(id_len);
    frame.insert(frame.end(), station_id.begin(), station_id.begin() + id_len);
    frame.push_back(static_cast<std::uint8_t>(new_mode));
    return frame;
}

[[nodiscard]] inline bool decode_mode_change(const std::uint8_t* data, std::size_t length, std::string& station_id, ModulationMode& new_mode) {
    if (length < 2 || data == nullptr) {
        return false;
    }
    std::size_t pos             = 0;
    std::uint8_t id_len          = data[pos++];
    if (pos + id_len + 1 > length) {
        return false;
    }
    station_id.assign(reinterpret_cast<const char*>(&data[pos]), id_len);
    pos += id_len;
    new_mode = static_cast<ModulationMode>(data[pos++]);
    return true;
}

[[nodiscard]] inline std::vector<std::uint8_t>
encode_quality_feedback(const std::string& station_id, float snr_db, float ber, float quality_score) {
    std::vector<std::uint8_t> frame;
    const std::uint8_t id_len = static_cast<std::uint8_t>(std::min<std::size_t>(255UZ, station_id.length()));
    frame.push_back(id_len);
    frame.insert(frame.end(), station_id.begin(), station_id.begin() + id_len);
    auto appendLe32 = [&](float x) {
        std::uint32_t bits{};
        std::memcpy(&bits, &x, sizeof(float));
        frame.push_back(static_cast<std::uint8_t>(bits & 0xFFU));
        frame.push_back(static_cast<std::uint8_t>((bits >> 8U) & 0xFFU));
        frame.push_back(static_cast<std::uint8_t>((bits >> 16U) & 0xFFU));
        frame.push_back(static_cast<std::uint8_t>((bits >> 24U) & 0xFFU));
    };
    appendLe32(snr_db);
    appendLe32(ber);
    appendLe32(quality_score);
    return frame;
}

[[nodiscard]] inline bool decode_quality_feedback(const std::uint8_t* data,
                                                  std::size_t length,
                                                  std::string& station_id,
                                                  float& snr_db,
                                                  float& ber,
                                                  float& quality_score) {
    if (length < 14 || data == nullptr) {
        return false;
    }
    std::size_t pos    = 0;
    std::uint8_t id_len = data[pos++];
    if (pos + id_len + 12 > length) {
        return false;
    }
    station_id.assign(reinterpret_cast<const char*>(&data[pos]), id_len);
    pos += id_len;
    auto readFloat = [&](float& out) -> bool {
        if (pos + 4 > length) return false;
        std::uint32_t bits =
            static_cast<std::uint32_t>(data[pos]) | (static_cast<std::uint32_t>(data[pos + 1]) << 8U)
            | (static_cast<std::uint32_t>(data[pos + 2]) << 16U) | (static_cast<std::uint32_t>(data[pos + 3]) << 24U);
        pos += 4;
        std::memcpy(&out, &bits, sizeof(float));
        return true;
    };
    return readFloat(snr_db) && readFloat(ber) && readFloat(quality_score);
}

} // namespace gnuradio4::packet_protocols::detail

#endif

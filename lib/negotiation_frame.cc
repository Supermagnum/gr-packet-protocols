/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "negotiation_frame.h"
#include <cstring>
#include <algorithm>

namespace gr {
namespace packet_protocols {

std::vector<uint8_t> encode_negotiation_request(const std::string& station_id,
                                                 modulation_mode_t proposed_mode,
                                                 const std::vector<modulation_mode_t>& supported_modes) {
    std::vector<uint8_t> frame;
    
    // Station ID length and ID
    uint8_t id_len = std::min(static_cast<size_t>(255), station_id.length());
    frame.push_back(id_len);
    frame.insert(frame.end(), station_id.begin(), station_id.begin() + id_len);
    
    // Proposed mode
    frame.push_back(static_cast<uint8_t>(proposed_mode));
    
    // Number of supported modes (max 8)
    uint8_t num_modes = std::min(static_cast<size_t>(8), supported_modes.size());
    frame.push_back(num_modes);
    
    // Supported modes
    for (size_t i = 0; i < num_modes; i++) {
        frame.push_back(static_cast<uint8_t>(supported_modes[i]));
    }
    
    return frame;
}

bool decode_negotiation_request(const uint8_t* data, size_t length, std::string& station_id,
                                modulation_mode_t& proposed_mode,
                                std::vector<modulation_mode_t>& supported_modes) {
    if (length < 3) {
        return false; // Need at least: id_len(1) + proposed_mode(1) + num_modes(1)
    }
    
    size_t pos = 0;
    
    // Station ID
    uint8_t id_len = data[pos++];
    if (pos + id_len + 2 > length) {
        return false;
    }
    station_id.assign(reinterpret_cast<const char*>(&data[pos]), id_len);
    pos += id_len;
    
    // Proposed mode
    proposed_mode = static_cast<modulation_mode_t>(data[pos++]);
    
    // Number of supported modes
    uint8_t num_modes = data[pos++];
    if (pos + num_modes > length) {
        return false;
    }
    
    // Supported modes
    supported_modes.clear();
    for (uint8_t i = 0; i < num_modes; i++) {
        supported_modes.push_back(static_cast<modulation_mode_t>(data[pos++]));
    }
    
    return true;
}

std::vector<uint8_t> encode_negotiation_response(const std::string& station_id, bool accepted,
                                                  modulation_mode_t negotiated_mode) {
    std::vector<uint8_t> frame;
    
    // Station ID length and ID
    uint8_t id_len = std::min(static_cast<size_t>(255), station_id.length());
    frame.push_back(id_len);
    frame.insert(frame.end(), station_id.begin(), station_id.begin() + id_len);
    
    // Accepted flag
    frame.push_back(accepted ? 1 : 0);
    
    // Negotiated mode
    frame.push_back(static_cast<uint8_t>(negotiated_mode));
    
    return frame;
}

bool decode_negotiation_response(const uint8_t* data, size_t length, std::string& station_id,
                                 bool& accepted, modulation_mode_t& negotiated_mode) {
    if (length < 3) {
        return false; // Need at least: id_len(1) + accepted(1) + mode(1)
    }
    
    size_t pos = 0;
    
    // Station ID
    uint8_t id_len = data[pos++];
    if (pos + id_len + 2 > length) {
        return false;
    }
    station_id.assign(reinterpret_cast<const char*>(&data[pos]), id_len);
    pos += id_len;
    
    // Accepted flag
    accepted = (data[pos++] != 0);
    
    // Negotiated mode
    negotiated_mode = static_cast<modulation_mode_t>(data[pos++]);
    
    return true;
}

std::vector<uint8_t> encode_mode_change(const std::string& station_id, modulation_mode_t new_mode) {
    std::vector<uint8_t> frame;
    
    // Station ID length and ID
    uint8_t id_len = std::min(static_cast<size_t>(255), station_id.length());
    frame.push_back(id_len);
    frame.insert(frame.end(), station_id.begin(), station_id.begin() + id_len);
    
    // New mode
    frame.push_back(static_cast<uint8_t>(new_mode));
    
    return frame;
}

bool decode_mode_change(const uint8_t* data, size_t length, std::string& station_id,
                        modulation_mode_t& new_mode) {
    if (length < 2) {
        return false; // Need at least: id_len(1) + mode(1)
    }
    
    size_t pos = 0;
    
    // Station ID
    uint8_t id_len = data[pos++];
    if (pos + id_len + 1 > length) {
        return false;
    }
    station_id.assign(reinterpret_cast<const char*>(&data[pos]), id_len);
    pos += id_len;
    
    // New mode
    new_mode = static_cast<modulation_mode_t>(data[pos++]);
    
    return true;
}

std::vector<uint8_t> encode_quality_feedback(const std::string& station_id, float snr_db, float ber,
                                             float quality_score) {
    std::vector<uint8_t> frame;
    
    // Station ID length and ID
    uint8_t id_len = std::min(static_cast<size_t>(255), station_id.length());
    frame.push_back(id_len);
    frame.insert(frame.end(), station_id.begin(), station_id.begin() + id_len);
    
    // SNR (4 bytes, IEEE 754 float)
    uint32_t snr_bits;
    std::memcpy(&snr_bits, &snr_db, sizeof(float));
    frame.push_back(snr_bits & 0xFF);
    frame.push_back((snr_bits >> 8) & 0xFF);
    frame.push_back((snr_bits >> 16) & 0xFF);
    frame.push_back((snr_bits >> 24) & 0xFF);
    
    // BER (4 bytes, IEEE 754 float)
    uint32_t ber_bits;
    std::memcpy(&ber_bits, &ber, sizeof(float));
    frame.push_back(ber_bits & 0xFF);
    frame.push_back((ber_bits >> 8) & 0xFF);
    frame.push_back((ber_bits >> 16) & 0xFF);
    frame.push_back((ber_bits >> 24) & 0xFF);
    
    // Quality score (4 bytes, IEEE 754 float)
    uint32_t quality_bits;
    std::memcpy(&quality_bits, &quality_score, sizeof(float));
    frame.push_back(quality_bits & 0xFF);
    frame.push_back((quality_bits >> 8) & 0xFF);
    frame.push_back((quality_bits >> 16) & 0xFF);
    frame.push_back((quality_bits >> 24) & 0xFF);
    
    return frame;
}

bool decode_quality_feedback(const uint8_t* data, size_t length, std::string& station_id,
                             float& snr_db, float& ber, float& quality_score) {
    if (length < 14) {
        return false; // Need at least: id_len(1) + id + snr(4) + ber(4) + quality(4)
    }
    
    size_t pos = 0;
    
    // Station ID
    uint8_t id_len = data[pos++];
    if (pos + id_len + 12 > length) {
        return false;
    }
    station_id.assign(reinterpret_cast<const char*>(&data[pos]), id_len);
    pos += id_len;
    
    // SNR
    uint32_t snr_bits = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) |
                        (data[pos + 3] << 24);
    std::memcpy(&snr_db, &snr_bits, sizeof(float));
    pos += 4;
    
    // BER
    uint32_t ber_bits = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) |
                        (data[pos + 3] << 24);
    std::memcpy(&ber, &ber_bits, sizeof(float));
    pos += 4;
    
    // Quality score
    uint32_t quality_bits = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) |
                            (data[pos + 3] << 24);
    std::memcpy(&quality_score, &quality_bits, sizeof(float));
    
    return true;
}

} // namespace packet_protocols
} // namespace gr


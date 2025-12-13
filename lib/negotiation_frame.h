/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 */

#ifndef INCLUDED_PACKET_PROTOCOLS_NEGOTIATION_FRAME_H
#define INCLUDED_PACKET_PROTOCOLS_NEGOTIATION_FRAME_H

#include <gnuradio/packet_protocols/adaptive_rate_control.h>
#include <gnuradio/packet_protocols/modulation_negotiation.h>
#include <string>
#include <vector>

namespace gr {
namespace packet_protocols {

// Encode negotiation request frame
// Format: [station_id_len(1)][station_id][proposed_mode(1)][num_modes(1)][modes...]
std::vector<uint8_t> encode_negotiation_request(const std::string& station_id,
                                                 modulation_mode_t proposed_mode,
                                                 const std::vector<modulation_mode_t>& supported_modes);

// Decode negotiation request frame
bool decode_negotiation_request(const uint8_t* data, size_t length, std::string& station_id,
                                modulation_mode_t& proposed_mode,
                                std::vector<modulation_mode_t>& supported_modes);

// Encode negotiation response frame
// Format: [station_id_len(1)][station_id][accepted(1)][negotiated_mode(1)]
std::vector<uint8_t> encode_negotiation_response(const std::string& station_id, bool accepted,
                                                  modulation_mode_t negotiated_mode);

// Decode negotiation response frame
bool decode_negotiation_response(const uint8_t* data, size_t length, std::string& station_id,
                                 bool& accepted, modulation_mode_t& negotiated_mode);

// Encode mode change notification
// Format: [station_id_len(1)][station_id][new_mode(1)]
std::vector<uint8_t> encode_mode_change(const std::string& station_id, modulation_mode_t new_mode);

// Decode mode change notification
bool decode_mode_change(const uint8_t* data, size_t length, std::string& station_id,
                        modulation_mode_t& new_mode);

// Encode quality feedback frame
// Format: [station_id_len(1)][station_id][snr_db(4)][ber(4)][quality_score(4)]
std::vector<uint8_t> encode_quality_feedback(const std::string& station_id, float snr_db, float ber,
                                             float quality_score);

// Decode quality feedback frame
bool decode_quality_feedback(const uint8_t* data, size_t length, std::string& station_id,
                             float& snr_db, float& ber, float& quality_score);

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_NEGOTIATION_FRAME_H */


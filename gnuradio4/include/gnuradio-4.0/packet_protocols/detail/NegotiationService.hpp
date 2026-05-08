// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_DETAIL_NEGOTIATION_SERVICE_HPP
#define GNURADIO4_PACKET_PROTOCOLS_DETAIL_NEGOTIATION_SERVICE_HPP

#include <gnuradio-4.0/packet_protocols/detail/ModulationMode.hpp>
#include <gnuradio-4.0/packet_protocols/detail/NegotiationFrame.hpp>
#include <gnuradio-4.0/packet_protocols/detail/ProtocolConstants.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gnuradio4::packet_protocols::detail {

using NegotiationEmit = std::function<void(std::uint8_t command, std::vector<std::uint8_t>&& frame_body)>;

inline void dispatch_negotiation_command(const std::string&                                station_identifier,
                                         const std::vector<ModulationMode>&              supported_modes,
                                         ModulationMode&                                   negotiated_mode,
                                         std::unordered_map<std::string, ModulationMode>& peers,
                                         std::uint8_t                                      kiss_cmd,
                                         const std::uint8_t*                               payload_bytes,
                                         std::size_t                                       payload_length,
                                         const NegotiationEmit& emit) {
    switch (kiss_cmd) {
    case kiss_cmd_neg_req: {
        std::string rid;
        ModulationMode proposed{};
        std::vector<ModulationMode> remote_supported{};
        if (!decode_negotiation_request(payload_bytes, payload_length, rid, proposed, remote_supported)) {
            return;
        }
        bool ok   = std::find(supported_modes.begin(), supported_modes.end(), proposed) != supported_modes.end();
        ModulationMode pick = negotiated_mode;
        if (ok) {
            pick               = proposed;
            negotiated_mode    = proposed;
            peers[rid]       = proposed;
        } else {
            for (auto mode : remote_supported) {
                if (std::find(supported_modes.begin(), supported_modes.end(), mode) != supported_modes.end()) {
                    pick            = mode;
                    negotiated_mode = mode;
                    peers[rid]    = mode;
                    ok              = true;
                    break;
                }
            }
        }
        emit(kiss_cmd_neg_resp, encode_negotiation_response(station_identifier, ok, pick));
        return;
    }
    case kiss_cmd_neg_resp: {
        std::string rid;
        bool        accepted{};
        ModulationMode mode{};
        if (!decode_negotiation_response(payload_bytes, payload_length, rid, accepted, mode)) {
            return;
        }
        if (!accepted) {
            return;
        }
        negotiated_mode = mode;
        peers[rid]      = mode;
        emit(kiss_cmd_neg_ack, encode_mode_change(station_identifier, mode));
        return;
    }
    case kiss_cmd_mode_ch: {
        std::string rid;
        ModulationMode nm{};
        if (!decode_mode_change(payload_bytes, payload_length, rid, nm)) {
            return;
        }
        peers[rid] = nm;
        return;
    }
    default:
        return;
    }
}

} // namespace gnuradio4::packet_protocols::detail

#endif

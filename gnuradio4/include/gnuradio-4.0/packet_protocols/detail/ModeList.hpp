// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_DETAIL_MODE_LIST_HPP
#define GNURADIO4_PACKET_PROTOCOLS_DETAIL_MODE_LIST_HPP

#include <gnuradio-4.0/packet_protocols/detail/ModulationMode.hpp>

#include <cctype>

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace gnuradio4::packet_protocols::detail {

inline std::vector<ModulationMode> parse_csv_modes(const std::string& csv) {
    std::vector<ModulationMode> modes;
    std::stringstream          ss(csv);
    std::string                  token;
    while (std::getline(ss, token, ',')) {
        token.erase(token.begin(),
                    std::find_if(token.begin(), token.end(),
                                 [](unsigned char ch) { return std::isspace(ch) == 0; }));
        token.erase(std::find_if(token.rbegin(), token.rend(),
                                 [](unsigned char ch) { return std::isspace(ch) == 0; }).base(),
                    token.end());
        if (!token.empty()) {
            modes.push_back(static_cast<ModulationMode>(static_cast<std::uint8_t>(std::stoi(token))));
        }
    }
    if (modes.empty()) {
        modes.push_back(ModulationMode::mode_2fsk);
    }
    return modes;
}

} // namespace gnuradio4::packet_protocols::detail

#endif

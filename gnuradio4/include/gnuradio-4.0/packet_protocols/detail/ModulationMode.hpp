// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_DETAIL_MODULATION_MODE_HPP
#define GNURADIO4_PACKET_PROTOCOLS_DETAIL_MODULATION_MODE_HPP

#include <cstdint>

namespace gnuradio4::packet_protocols::detail {

enum class ModulationMode : std::uint8_t {
    mode_2fsk       = 0,
    mode_4fsk       = 1,
    mode_8fsk       = 2,
    mode_16fsk      = 3,
    mode_bpsk       = 4,
    mode_qpsk       = 5,
    mode_8psk       = 6,
    mode_qam16      = 7,
    mode_qam64_6250 = 8,
    mode_qam64_12500 = 9,
    mode_qam256     = 10,
    mode_bpsk_12500 = 11,
    mode_qpsk_12500 = 12,
    mode_8psk_12500 = 13,
    mode_qam16_12500 = 14,
    mode_soqpsk_1m  = 15,
    mode_soqpsk_5m  = 16,
    mode_soqpsk_10m = 17,
    mode_soqpsk_20m = 18,
    mode_soqpsk_40m = 19
};

[[nodiscard]] inline bool isTier4(ModulationMode m) noexcept {
    return m == ModulationMode::mode_soqpsk_1m || m == ModulationMode::mode_soqpsk_5m || m == ModulationMode::mode_soqpsk_10m
        || m == ModulationMode::mode_soqpsk_20m || m == ModulationMode::mode_soqpsk_40m;
}

} // namespace gnuradio4::packet_protocols::detail

#endif

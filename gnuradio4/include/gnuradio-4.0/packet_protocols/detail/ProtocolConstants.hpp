// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_DETAIL_PROTOCOL_CONSTANTS_HPP
#define GNURADIO4_PACKET_PROTOCOLS_DETAIL_PROTOCOL_CONSTANTS_HPP

#include <cstdint>

namespace gnuradio4::packet_protocols::detail {

inline constexpr std::uint8_t kiss_fend       = 0xC0U;
inline constexpr std::uint8_t kiss_fesc       = 0xDBU;
inline constexpr std::uint8_t kiss_tfend      = 0xDCU;
inline constexpr std::uint8_t kiss_tfesc      = 0xDDU;
inline constexpr std::uint8_t kiss_cmd_data   = 0x00U;
inline constexpr std::uint8_t kiss_cmd_neg_req  = 0x10U;
inline constexpr std::uint8_t kiss_cmd_neg_resp = 0x11U;
inline constexpr std::uint8_t kiss_cmd_neg_ack  = 0x12U;
inline constexpr std::uint8_t kiss_cmd_mode_ch  = 0x13U;
inline constexpr std::uint8_t kiss_cmd_qual_fb = 0x14U;

inline constexpr std::uint8_t fx25_fec_rs255_239 = 0x01U;
inline constexpr std::uint8_t fx25_fec_rs255_223 = 0x02U;
inline constexpr std::uint8_t fx25_fec_rs255_191 = 0x03U;
inline constexpr std::uint8_t fx25_fec_rs255_159 = 0x04U;
inline constexpr std::uint8_t fx25_fec_rs255_127 = 0x05U;
inline constexpr std::uint8_t fx25_fec_rs255_95  = 0x06U;
inline constexpr std::uint8_t fx25_fec_rs255_63  = 0x07U;
inline constexpr std::uint8_t fx25_fec_rs255_31  = 0x08U;

inline constexpr std::uint8_t il2p_fec_rs255_223 = 0x01U;
inline constexpr std::uint8_t il2p_fec_rs255_239 = 0x02U;
inline constexpr std::uint8_t il2p_fec_rs255_247 = 0x03U;

inline constexpr std::uint8_t il2p_preamble   = 0x55U;
inline constexpr std::uint32_t il2p_sync_be = 0xF15E48U;

} // namespace gnuradio4::packet_protocols::detail

#endif

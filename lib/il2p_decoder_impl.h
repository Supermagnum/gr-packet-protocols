/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 *
 * gr-packet-protocols is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * gr-packet-protocols is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gr-packet-protocols; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_PACKET_PROTOCOLS_IL2P_DECODER_IMPL_H
#define INCLUDED_PACKET_PROTOCOLS_IL2P_DECODER_IMPL_H

#include <gnuradio/packet_protocols/common.h> // Include common.h for ReedSolomonDecoder and FEC types
#include <gnuradio/packet_protocols/il2p_decoder.h>
#include <gnuradio/packet_protocols/il2p_protocol.h>
#include <string>
#include <vector>

namespace gr {
namespace packet_protocols {

// State machine constants
enum il2p_state_t { STATE_IDLE = 0, STATE_FLAG = 1, STATE_DATA = 2, STATE_FRAME_COMPLETE = 3 };

class il2p_decoder_impl : public il2p_decoder {
  private:
    il2p_state_t d_state;                       //!< Current state
    uint8_t d_bit_buffer;                       //!< Bit buffer for byte assembly
    uint8_t d_bit_count;                        //!< Number of bits in buffer
    std::vector<uint8_t> d_frame_buffer;        //!< Frame buffer
    uint16_t d_frame_length;                    //!< Current frame length
    uint8_t d_ones_count;                       //!< Consecutive ones count
    bool d_escaped;                             //!< Escape flag for bit stuffing
    int d_fec_type;                             //!< FEC type
    ReedSolomonDecoder* d_reed_solomon_decoder; //!< Reed-Solomon decoder

  public:
    il2p_decoder_impl();
    ~il2p_decoder_impl();

    int work(int noutput_items, gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);

  private:
    void process_bit(bool bit);
    void initialize_reed_solomon();
    std::vector<uint8_t> decode_il2p_frame();
    bool parse_il2p_header();
    std::vector<uint8_t> apply_reed_solomon_decode(const std::vector<uint8_t>& data);
    bool validate_checksum();
    uint32_t calculate_checksum();
    std::string extract_callsign(int start_pos);
    int extract_ssid(int pos);
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_IL2P_DECODER_IMPL_H */

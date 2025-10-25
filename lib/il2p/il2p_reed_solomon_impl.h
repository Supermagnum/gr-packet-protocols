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

#ifndef INCLUDED_PACKET_PROTOCOLS_IL2P_REED_SOLOMON_IMPL_H
#define INCLUDED_PACKET_PROTOCOLS_IL2P_REED_SOLOMON_IMPL_H

#include <gnuradio/packet_protocols/common.h> // Include common.h for ReedSolomonEncoder/Decoder and FEC types
#include <gnuradio/packet_protocols/il2p_protocol.h>
#include <gnuradio/packet_protocols/il2p_reed_solomon.h>
#include <vector>

namespace gr {
namespace packet_protocols {

class il2p_reed_solomon_impl : public il2p_reed_solomon {
  private:
    int d_fec_type;                             //!< FEC type
    bool d_encode_mode;                         //!< Encode or decode mode
    ReedSolomonEncoder* d_reed_solomon_encoder; //!< Reed-Solomon encoder
    ReedSolomonDecoder* d_reed_solomon_decoder; //!< Reed-Solomon decoder
    std::vector<uint8_t> d_frame_buffer;        //!< Frame buffer
    uint16_t d_frame_length;                    //!< Current frame length
    uint8_t d_bit_position;                     //!< Current bit position in byte
    uint16_t d_byte_position;                   //!< Current byte position in frame

  public:
    il2p_reed_solomon_impl(int fec_type, bool encode_mode);
    ~il2p_reed_solomon_impl();

    int work(int noutput_items, gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);

    // Virtual functions from base class
    void set_fec_type(int fec_type);
    void set_encode_mode(bool encode_mode);
    int get_data_length() const;
    int get_code_length() const;
    int get_error_correction_capability() const;

  private:
    void initialize_reed_solomon();
    void build_encoded_frame(char data_byte);
    void build_decoded_frame(char data_byte);
    std::vector<uint8_t> apply_reed_solomon_encode(const std::vector<uint8_t>& data);
    std::vector<uint8_t> apply_reed_solomon_decode(const std::vector<uint8_t>& data);
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_IL2P_REED_SOLOMON_IMPL_H */

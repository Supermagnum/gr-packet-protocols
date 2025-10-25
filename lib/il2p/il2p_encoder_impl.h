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

#ifndef INCLUDED_PACKET_PROTOCOLS_IL2P_ENCODER_IMPL_H
#define INCLUDED_PACKET_PROTOCOLS_IL2P_ENCODER_IMPL_H

#include <gnuradio/packet_protocols/common.h>
#include <gnuradio/packet_protocols/il2p_encoder.h>
#include <gnuradio/packet_protocols/il2p_protocol.h>
#include <string>
#include <vector>

namespace gr {
namespace packet_protocols {

/*!
 * \brief IL2P Encoder Implementation
 * \ingroup packet_protocols
 *
 * This class implements IL2P packet encoding using the real
 * IL2P protocol implementation from gr-m17.
 */
class il2p_encoder_impl : public il2p_encoder {
  private:
    std::string d_dest_callsign;                //!< Destination callsign
    std::string d_dest_ssid;                    //!< Destination SSID
    std::string d_src_callsign;                 //!< Source callsign
    std::string d_src_ssid;                     //!< Source SSID
    int d_fec_type;                             //!< FEC type
    bool d_add_checksum;                        //!< Add checksum flag
    std::vector<uint8_t> d_frame_buffer;        //!< Frame buffer
    uint16_t d_frame_length;                    //!< Current frame length
    uint8_t d_bit_position;                     //!< Current bit position
    uint16_t d_byte_position;                   //!< Current byte position
    ReedSolomonEncoder* d_reed_solomon_encoder; //!< Reed-Solomon encoder

  public:
    /*!
     * \brief Constructor
     * \param dest_callsign Destination callsign
     * \param dest_ssid Destination SSID
     * \param src_callsign Source callsign
     * \param src_ssid Source SSID
     * \param fec_type FEC type
     * \param add_checksum Add checksum flag
     */
    il2p_encoder_impl(const std::string& dest_callsign, const std::string& dest_ssid,
                      const std::string& src_callsign, const std::string& src_ssid, int fec_type,
                      bool add_checksum);

    /*!
     * \brief Destructor
     */
    ~il2p_encoder_impl();

    /*!
     * \brief Main work function
     * \param noutput_items Number of output items
     * \param input_items Input items
     * \param output_items Output items
     * \return Number of items produced
     */
    int work(int noutput_items, gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);

    /*!
     * \brief Set FEC type
     * \param fec_type FEC type value
     */
    void set_fec_type(int fec_type);

    /*!
     * \brief Set add checksum flag
     * \param add_checksum Add checksum flag
     */
    void set_add_checksum(bool add_checksum);

  private:
    /*!
     * \brief Initialize Reed-Solomon encoder
     */
    void initialize_reed_solomon();

    /*!
     * \brief Build IL2P frame
     * \param data_byte Input data byte
     */
    void build_il2p_frame(char data_byte);

    /*!
     * \brief Add IL2P header to frame
     */
    void add_il2p_header();

    /*!
     * \brief Add address to frame
     * \param callsign Callsign
     * \param ssid SSID
     * \param is_last Is last address flag
     */
    void add_address(const std::string& callsign, const std::string& ssid, bool is_last);

    /*!
     * \brief Apply Reed-Solomon FEC to data
     * \param data Input data
     * \return FEC encoded data
     */
    std::vector<uint8_t> apply_reed_solomon_fec(const std::vector<uint8_t>& data);

    /*!
     * \brief Calculate checksum for frame
     * \return Calculated checksum
     */
    uint32_t calculate_checksum();
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_IL2P_ENCODER_IMPL_H */
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

#ifndef INCLUDED_PACKET_PROTOCOLS_AX25_ENCODER_IMPL_H
#define INCLUDED_PACKET_PROTOCOLS_AX25_ENCODER_IMPL_H

#include <gnuradio/packet_protocols/ax25_encoder.h>
#include <gnuradio/packet_protocols/ax25_protocol.h>
#include <vector>

namespace gr {
namespace packet_protocols {

/*!
 * \brief AX.25 Encoder Implementation
 * \ingroup packet_protocols
 *
 * This class implements AX.25 packet encoding using the real
 * AX.25 protocol implementation from gr-m17.
 */
class ax25_encoder_impl : public ax25_encoder {
  private:
    std::string d_dest_callsign; //!< Destination callsign
    std::string d_dest_ssid;     //!< Destination SSID
    std::string d_src_callsign;  //!< Source callsign
    std::string d_src_ssid;      //!< Source SSID
    std::string d_digipeaters;   //!< Digipeater list
    bool d_command_response;     //!< Command/Response flag
    bool d_poll_final;           //!< Poll/Final flag

    ax25_tnc_t d_tnc;                    //!< AX.25 TNC context
    std::vector<uint8_t> d_frame_buffer; //!< Encoded frame bytes (scratch, incl. HDLC flags)
    /** Serialized bits to emit: opening/closing flags raw MSB-first; body HDLC bit-stuffed */
    std::vector<uint8_t> d_bit_queue;
    size_t d_bit_q_read{ 0 }; //!< Read index into d_bit_queue

  public:
    /*!
     * \brief Constructor
     * \param dest_callsign Destination callsign
     * \param dest_ssid Destination SSID
     * \param src_callsign Source callsign
     * \param src_ssid Source SSID
     * \param digipeaters Digipeater list (comma-separated)
     * \param command_response Command/Response flag
     * \param poll_final Poll/Final flag
     */
    ax25_encoder_impl(const std::string& dest_callsign, const std::string& dest_ssid,
                      const std::string& src_callsign, const std::string& src_ssid,
                      const std::string& digipeaters, bool command_response, bool poll_final);

    /*!
     * \brief Destructor
     */
    ~ax25_encoder_impl();

    /*!
     * \brief Estimate input bytes needed for a produce request
     */
    void forecast(int noutput_items, gr_vector_int& ninput_items_required) override;

    /*!
     * \brief Expand payload bytes to serialized HDLC bits (non 1:1 rate)
     */
    int general_work(int noutput_items,
                     gr_vector_int& ninput_items,
                     gr_vector_const_void_star& input_items,
                     gr_vector_void_star& output_items) override;

  private:
    /*!
     * \brief Build AX.25 frame from input data
     * \param data_byte Input data byte
     */
    void build_ax25_frame(char data_byte);
    static void push_msb_bits_raw(uint8_t byte, std::vector<uint8_t>& q);
    static void push_msb_bits_stuffed(uint8_t byte, std::vector<uint8_t>& q, int& ones_run);
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_AX25_ENCODER_IMPL_H */

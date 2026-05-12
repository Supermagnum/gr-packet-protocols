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

#ifndef INCLUDED_PACKET_PROTOCOLS_AX25_DECODER_IMPL_H
#define INCLUDED_PACKET_PROTOCOLS_AX25_DECODER_IMPL_H

#include <deque>
#include <gnuradio/packet_protocols/ax25_decoder.h>
#include <gnuradio/packet_protocols/ax25_protocol.h>
#include <vector>

namespace gr {
namespace packet_protocols {

// State machine constants
enum ax25_state_t { STATE_IDLE = 0, STATE_FLAG = 1, STATE_DATA = 2, STATE_FRAME_COMPLETE = 3 };

/*!
 * \brief AX.25 Decoder Implementation
 * \ingroup packet_protocols
 *
 * This class implements AX.25 packet decoding using the real
 * AX.25 protocol implementation from gr-m17.
 */
class ax25_decoder_impl : public ax25_decoder {
  private:
    ax25_state_t d_state;                //!< Current state
    uint8_t d_bit_buffer;                //!< Bit buffer for byte assembly
    uint8_t d_bit_count;                 //!< Number of bits in buffer
    std::vector<uint8_t> d_frame_buffer; //!< Frame buffer
    uint16_t d_frame_length;             //!< Current frame length
    uint8_t d_ones_count;                //!< Consecutive ones count
    bool d_escaped;                      //!< Escape flag for bit stuffing

    std::deque<uint8_t> d_out_queue; //!< Pending PDU bytes (handles output buffer smaller than PDU)

  public:
    /*!
     * \brief Constructor
     */
    ax25_decoder_impl();

    /*!
     * \brief Destructor
     */
    ~ax25_decoder_impl();

    /*!
     * \brief Estimate input bits needed for a produce request
     */
    void forecast(int noutput_items, gr_vector_int& ninput_items_required) override;

    /*!
     * \brief Deserialize HDLC bits to AX.25 PDU bytes (non 1:1 rate)
     */
    int general_work(int noutput_items,
                     gr_vector_int& ninput_items,
                     gr_vector_const_void_star& input_items,
                     gr_vector_void_star& output_items) override;

  private:
    /*!
     * \brief Process a single bit through the state machine
     * \param bit Input bit
     */
    void process_bit(bool bit);

    /*!
     * \brief Validate the received frame
     * \return true if frame is valid
     */
    bool validate_frame();

    /*!
     * \brief Calculate FCS for frame validation
     * \return Calculated FCS value
     */
    uint16_t calculate_fcs();

    /*!
     * \brief Extract callsign from frame data
     * \param start_pos Starting position in frame
     * \return Extracted callsign string
     */
    std::string extract_callsign(int start_pos);

    /*!
     * \brief Extract SSID from frame data
     * \param pos Position in frame
     * \return Extracted SSID value
     */
    int extract_ssid(int pos);
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_AX25_DECODER_IMPL_H */

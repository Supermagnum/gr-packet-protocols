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

#ifndef INCLUDED_PACKET_PROTOCOLS_FX25_ENCODER_IMPL_H
#define INCLUDED_PACKET_PROTOCOLS_FX25_ENCODER_IMPL_H

#include <gnuradio/packet_protocols/common.h>
#include <gnuradio/packet_protocols/fx25_encoder.h>
#include <gnuradio/packet_protocols/fx25_protocol.h>
#include <vector>

namespace gr {
namespace packet_protocols {

/*!
 * \brief FX.25 Encoder Implementation
 * \ingroup packet_protocols
 *
 * This class implements FX.25 packet encoding using the real
 * FX.25 protocol implementation from gr-m17.
 */
class fx25_encoder_impl : public fx25_encoder {
  private:
    int d_fec_type;                             //!< FEC type
    int d_interleaver_depth;                    //!< Interleaver depth
    bool d_add_checksum;                        //!< Add checksum flag
    std::vector<uint8_t> d_frame_buffer;        //!< Frame buffer
    uint16_t d_frame_length;                    //!< Current frame length
    uint8_t d_bit_position;                     //!< Current bit position
    uint16_t d_byte_position;                   //!< Current byte position
    ReedSolomonEncoder* d_reed_solomon_encoder; //!< Reed-Solomon encoder

  public:
    /*!
     * \brief Constructor
     * \param fec_type FEC type
     * \param interleaver_depth Interleaver depth
     * \param add_checksum Add checksum flag
     */
    fx25_encoder_impl(int fec_type, int interleaver_depth, bool add_checksum);

    /*!
     * \brief Destructor
     */
    ~fx25_encoder_impl();

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
     * \brief Set interleaver depth
     * \param depth Interleaver depth value
     */
    void set_interleaver_depth(int depth);

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
     * \brief Build FX.25 frame
     * \param data_byte Input data byte
     */
    void build_fx25_frame(char data_byte);

    /*!
     * \brief Add FX.25 header to frame
     */
    void add_fx25_header();

    /*!
     * \brief Apply Reed-Solomon FEC to data
     * \param data Input data
     * \return FEC encoded data
     */
    std::vector<uint8_t> apply_reed_solomon_fec(const std::vector<uint8_t>& data);

    /*!
     * \brief Interleave data
     * \param data Input data
     * \return Interleaved data
     */
    std::vector<uint8_t> interleave_data(const std::vector<uint8_t>& data);

    /*!
     * \brief Calculate checksum for frame
     * \return Calculated checksum
     */
    uint16_t calculate_checksum();
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_FX25_ENCODER_IMPL_H */
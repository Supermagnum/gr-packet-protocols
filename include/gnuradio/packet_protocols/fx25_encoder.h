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

#ifndef INCLUDED_PACKET_PROTOCOLS_FX25_ENCODER_H
#define INCLUDED_PACKET_PROTOCOLS_FX25_ENCODER_H

#include <gnuradio/packet_protocols/api.h>
#include <gnuradio/packet_protocols/common.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace packet_protocols {

/*!
 * \brief FX.25 Encoder with Forward Error Correction
 * \ingroup packet_protocols
 */
class PACKET_PROTOCOLS_API fx25_encoder : virtual public gr::sync_block {
  public:
    typedef std::shared_ptr<fx25_encoder> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of packet_protocols::fx25_encoder.
     */
    static sptr make(int fec_type = FX25_FEC_RS_16_12, int interleaver_depth = 1,
                     bool add_checksum = true);

    /*!
     * \brief Set FEC type
     */
    virtual void set_fec_type(int fec_type) = 0;

    /*!
     * \brief Set interleaver depth
     */
    virtual void set_interleaver_depth(int depth) = 0;

    /*!
     * \brief Set add checksum
     */
    virtual void set_add_checksum(bool add_checksum) = 0;
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_FX25_ENCODER_H */

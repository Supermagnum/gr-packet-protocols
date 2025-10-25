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

#ifndef INCLUDED_PACKET_PROTOCOLS_IL2P_REED_SOLOMON_H
#define INCLUDED_PACKET_PROTOCOLS_IL2P_REED_SOLOMON_H

#include <gnuradio/packet_protocols/api.h>
#include <gnuradio/packet_protocols/common.h> // Include common.h for IL2P_FEC_RS_255_223
#include <gnuradio/sync_block.h>

namespace gr {
namespace packet_protocols {

/*!
 * \brief IL2P Reed-Solomon Forward Error Correction
 * \ingroup packet_protocols
 */
class PACKET_PROTOCOLS_API il2p_reed_solomon : virtual public gr::sync_block {
  public:
    typedef std::shared_ptr<il2p_reed_solomon> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of packet_protocols::il2p_reed_solomon.
     */
    static sptr make(int fec_type = IL2P_FEC_RS_255_223, bool encode_mode = true);

    /*!
     * \brief Set FEC type
     */
    virtual void set_fec_type(int fec_type) = 0;

    /*!
     * \brief Set encode mode
     */
    virtual void set_encode_mode(bool encode_mode) = 0;

    /*!
     * \brief Get data length
     */
    virtual int get_data_length() const = 0;

    /*!
     * \brief Get code length
     */
    virtual int get_code_length() const = 0;

    /*!
     * \brief Get error correction capability
     */
    virtual int get_error_correction_capability() const = 0;
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_IL2P_REED_SOLOMON_H */

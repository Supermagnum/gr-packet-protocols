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

#ifndef INCLUDED_PACKET_PROTOCOLS_AX25_ENCODER_H
#define INCLUDED_PACKET_PROTOCOLS_AX25_ENCODER_H

#include <gnuradio/packet_protocols/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace packet_protocols {

/*!
 * \brief AX.25 Encoder
 * \ingroup packet_protocols
 */
class PACKET_PROTOCOLS_API ax25_encoder : virtual public gr::sync_block {
  public:
    typedef std::shared_ptr<ax25_encoder> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of packet_protocols::ax25_encoder.
     *
     * To avoid accidental use of raw pointers, packet_protocols::ax25_encoder's
     * constructor is in a private implementation
     * class. packet_protocols::ax25_encoder::make is the public interface for
     * creating new instances.
     */
    static sptr make(const std::string& dest_callsign, const std::string& dest_ssid,
                     const std::string& src_callsign, const std::string& src_ssid,
                     const std::string& digipeaters = "", bool command_response = false,
                     bool poll_final = false);
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_AX25_ENCODER_H */

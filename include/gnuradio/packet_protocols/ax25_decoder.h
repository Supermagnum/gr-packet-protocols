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

#ifndef INCLUDED_PACKET_PROTOCOLS_AX25_DECODER_H
#define INCLUDED_PACKET_PROTOCOLS_AX25_DECODER_H

#include <gnuradio/sync_block.h>
#include <packet_protocols/api.h>

namespace gr {
  namespace packet_protocols {

    /*!
     * \brief AX.25 Decoder
     * \ingroup packet_protocols
     */
    class PACKET_PROTOCOLS_API ax25_decoder : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<ax25_decoder> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of packet_protocols::ax25_decoder.
       */
      static sptr make();
    };

  } // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_AX25_DECODER_H */






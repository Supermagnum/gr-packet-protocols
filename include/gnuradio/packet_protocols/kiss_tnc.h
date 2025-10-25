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

#ifndef INCLUDED_PACKET_PROTOCOLS_KISS_TNC_H
#define INCLUDED_PACKET_PROTOCOLS_KISS_TNC_H

#include <gnuradio/packet_protocols/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace packet_protocols {

/*!
 * \brief KISS TNC Interface
 * \ingroup packet_protocols
 */
class PACKET_PROTOCOLS_API kiss_tnc : virtual public gr::sync_block {
  public:
    typedef std::shared_ptr<kiss_tnc> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of packet_protocols::kiss_tnc.
     */
    static sptr make(const std::string& device, int baud_rate = 9600,
                     bool hardware_flow_control = false);

    /*!
     * \brief Set TX delay
     */
    virtual void set_tx_delay(int delay) = 0;

    /*!
     * \brief Set persistence
     */
    virtual void set_persistence(int persistence) = 0;

    /*!
     * \brief Set slot time
     */
    virtual void set_slot_time(int slot_time) = 0;

    /*!
     * \brief Set TX tail
     */
    virtual void set_tx_tail(int tx_tail) = 0;

    /*!
     * \brief Set full duplex mode
     */
    virtual void set_full_duplex(bool full_duplex) = 0;
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_KISS_TNC_H */

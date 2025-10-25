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

#ifndef INCLUDED_PACKET_PROTOCOLS_KISS_TNC_IMPL_H
#define INCLUDED_PACKET_PROTOCOLS_KISS_TNC_IMPL_H

#include <gnuradio/packet_protocols/ax25_protocol.h>
#include <gnuradio/packet_protocols/kiss_tnc.h>
#include <string>
#include <vector>

namespace gr {
namespace packet_protocols {

// KISS state machine constants
enum kiss_state_t { KISS_STATE_IDLE = 0, KISS_STATE_FRAME = 1 };

// KISS command constants
enum kiss_cmd_t {
    KISS_CMD_DATA = 0,
    KISS_CMD_TXDELAY = 1,
    KISS_CMD_P = 2,
    KISS_CMD_SLOTTIME = 3,
    KISS_CMD_TXTAIL = 4,
    KISS_CMD_FULLDUPLEX = 5,
    KISS_CMD_SET_HARDWARE = 6,
    KISS_CMD_RETURN = 15
};

// KISS frame constants
const uint8_t KISS_FEND = 0xC0;
const uint8_t KISS_FESC = 0xDB;
const uint8_t KISS_TFEND = 0xDC;
const uint8_t KISS_TFESC = 0xDD;

/*!
 * \brief KISS TNC Implementation
 * \ingroup packet_protocols
 *
 * This class implements KISS TNC functionality using the real
 * AX.25 protocol implementation from gr-m17.
 */
class kiss_tnc_impl : public kiss_tnc {
  private:
    std::string d_device;                //!< Serial device path
    int d_baud_rate;                     //!< Baud rate
    bool d_hardware_flow_control;        //!< Hardware flow control flag
    int d_serial_fd;                     //!< Serial file descriptor
    kiss_state_t d_kiss_state;           //!< Current KISS state
    bool d_escape_next;                  //!< Escape next byte flag
    std::vector<uint8_t> d_frame_buffer; //!< Frame buffer
    uint16_t d_frame_length;             //!< Current frame length
    uint8_t d_ones_count;                //!< Consecutive ones count

    // KISS parameters
    int d_tx_delay;          //!< TX delay
    int d_persistence;       //!< Persistence
    int d_slot_time;         //!< Slot time
    int d_tx_tail;           //!< TX tail
    bool d_full_duplex;      //!< Full duplex mode
    uint8_t d_hardware_type; //!< Hardware type
    bool d_kiss_mode;        //!< KISS mode flag

  public:
    /*!
     * \brief Constructor
     * \param device Serial device path
     * \param baud_rate Baud rate
     * \param hardware_flow_control Hardware flow control flag
     */
    kiss_tnc_impl(const std::string& device, int baud_rate, bool hardware_flow_control);

    /*!
     * \brief Destructor
     */
    ~kiss_tnc_impl();

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
     * \brief Set TX delay
     * \param delay TX delay value
     */
    void set_tx_delay(int delay) override;

    /*!
     * \brief Set persistence
     * \param persistence Persistence value
     */
    void set_persistence(int persistence) override;

    /*!
     * \brief Set slot time
     * \param slot_time Slot time value
     */
    void set_slot_time(int slot_time) override;

    /*!
     * \brief Set TX tail
     * \param tx_tail TX tail value
     */
    void set_tx_tail(int tx_tail) override;

    /*!
     * \brief Set full duplex mode
     * \param full_duplex Full duplex flag
     */
    void set_full_duplex(bool full_duplex) override;

  private:
    /*!
     * \brief Open serial port
     * \return true if successful
     */
    bool open_serial_port();

    /*!
     * \brief Process KISS frame
     */
    void process_kiss_frame();

    /*!
     * \brief Send KISS frame
     * \param command KISS command
     * \param port Port number
     * \param data Data to send
     * \param length Data length
     */
    void send_kiss_frame(uint8_t command, uint8_t port, const uint8_t* data, int length);
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_KISS_TNC_IMPL_H */
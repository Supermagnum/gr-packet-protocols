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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ax25_encoder_impl.h"
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <gnuradio/io_signature.h>

namespace gr {
namespace packet_protocols {

ax25_encoder::sptr ax25_encoder::make(const std::string& dest_callsign,
                                      const std::string& dest_ssid, const std::string& src_callsign,
                                      const std::string& src_ssid, const std::string& digipeaters,
                                      bool command_response, bool poll_final) {
    return gnuradio::make_block_sptr<ax25_encoder_impl>(dest_callsign, dest_ssid, src_callsign,
                                                        src_ssid, digipeaters, command_response,
                                                        poll_final);
}

ax25_encoder_impl::ax25_encoder_impl(const std::string& dest_callsign, const std::string& dest_ssid,
                                     const std::string& src_callsign, const std::string& src_ssid,
                                     const std::string& digipeaters, bool command_response,
                                     bool poll_final)
    : gr::block("ax25_encoder", gr::io_signature::make(1, 1, sizeof(char)),
                gr::io_signature::make(1, 1, sizeof(char))),
      d_dest_callsign(dest_callsign), d_dest_ssid(dest_ssid), d_src_callsign(src_callsign),
      d_src_ssid(src_ssid), d_digipeaters(digipeaters), d_command_response(command_response),
      d_poll_final(poll_final), d_frame_buffer(), d_bit_queue(), d_bit_q_read(0) {
    // Initialize AX.25 TNC
    ax25_init(&d_tnc);

    // Set my address
    ax25_set_address(&d_tnc.config.my_address, src_callsign.c_str(), std::stoi(src_ssid), false);

    d_frame_buffer.clear();
}

ax25_encoder_impl::~ax25_encoder_impl() {
    ax25_cleanup(&d_tnc);
}

void ax25_encoder_impl::push_msb_bits_raw(uint8_t byte, std::vector<uint8_t>& q)
{
    for (int bp = 7; bp >= 0; --bp)
        q.push_back(static_cast<uint8_t>((byte >> bp) & 1));
}

void ax25_encoder_impl::push_msb_bits_stuffed(uint8_t byte, std::vector<uint8_t>& q, int& ones_run)
{
    for (int bp = 7; bp >= 0; --bp) {
        const bool bit = ((byte >> bp) & 1) != 0;
        q.push_back(bit ? 1 : 0);
        if (bit) {
            ones_run++;
            if (ones_run == 5) {
                q.push_back(0);
                ones_run = 0;
            }
        } else {
            ones_run = 0;
        }
    }
}

void ax25_encoder_impl::forecast(int noutput_items, gr_vector_int& ninput_items_required)
{
    if (d_bit_q_read < d_bit_queue.size()) {
        ninput_items_required[0] = 0;
        return;
    }
    ninput_items_required[0] = (noutput_items > 0) ? 1 : 0;
}

int ax25_encoder_impl::general_work(int noutput_items,
                                    gr_vector_int& ninput_items,
                                    gr_vector_const_void_star& input_items,
                                    gr_vector_void_star& output_items)
{
    const char* in = (const char*)input_items[0];
    char* out = (char*)output_items[0];
    int produced = 0;
    int consumed = 0;
    const int n_in = ninput_items[0];

    while (produced < noutput_items) {
        while (d_bit_q_read < d_bit_queue.size() && produced < noutput_items) {
            out[produced++] = static_cast<char>(d_bit_queue[d_bit_q_read++]);
        }
        if (produced >= noutput_items)
            break;
        if (consumed >= n_in)
            break;
        build_ax25_frame(in[consumed]);
        consumed++;
    }

    if (d_bit_q_read >= d_bit_queue.size() && !d_bit_queue.empty()) {
        d_bit_queue.clear();
        d_bit_q_read = 0;
    }

    consume_each(consumed);
    return produced;
}

void ax25_encoder_impl::build_ax25_frame(char data_byte)
{
    d_frame_buffer.clear();
    d_bit_queue.clear();
    d_bit_q_read = 0;

    // Create AX.25 frame using the real implementation
    ax25_frame_t frame;
    ax25_address_t dest_addr, src_addr;

    // Set up addresses
    ax25_set_address(&dest_addr, d_dest_callsign.c_str(), std::stoi(d_dest_ssid), true);
    ax25_set_address(&src_addr, d_src_callsign.c_str(), std::stoi(d_src_ssid), false);

    // Create UI frame (for APRS-style transmission)
    uint8_t info[1] = { static_cast<uint8_t>(data_byte) };
    ax25_create_frame(&frame, &src_addr, &dest_addr, AX25_CTRL_UI, AX25_PID_NONE, info, 1);

    // Encode frame
    uint8_t encoded[512];
    uint16_t encoded_len = sizeof(encoded);
    if (ax25_encode_frame(&frame, encoded, &encoded_len) != 0)
        return;
    if (ax25_add_flags(encoded, &encoded_len, sizeof(encoded)) != 0)
        return;
    if (encoded_len < 2)
        return;

    /* Wire format: raw HDLC flags (0x7E) MSB-first; stuff only the octets between flags */
    push_msb_bits_raw(encoded[0], d_bit_queue);
    int ones_run = 0;
    for (uint16_t i = 1; i + 1 < encoded_len; ++i)
        push_msb_bits_stuffed(encoded[i], d_bit_queue, ones_run);
    push_msb_bits_raw(encoded[encoded_len - 1], d_bit_queue);
}

} /* namespace packet_protocols */
} /* namespace gr */

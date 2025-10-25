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
    : gr::sync_block("ax25_encoder", gr::io_signature::make(1, 1, sizeof(char)),
                     gr::io_signature::make(1, 1, sizeof(char))),
      d_dest_callsign(dest_callsign), d_dest_ssid(dest_ssid), d_src_callsign(src_callsign),
      d_src_ssid(src_ssid), d_digipeaters(digipeaters), d_command_response(command_response),
      d_poll_final(poll_final), d_frame_buffer(1024), d_frame_length(0), d_bit_position(0),
      d_byte_position(0) {
    // Initialize AX.25 TNC
    ax25_init(&d_tnc);

    // Set my address
    ax25_set_address(&d_tnc.config.my_address, src_callsign.c_str(), std::stoi(src_ssid), false);

    d_frame_buffer.clear();
    d_frame_length = 0;
}

ax25_encoder_impl::~ax25_encoder_impl() {
    ax25_cleanup(&d_tnc);
}

int ax25_encoder_impl::work(int noutput_items, gr_vector_const_void_star& input_items,
                            gr_vector_void_star& output_items) {
    const char* in = (const char*)input_items[0];
    char* out = (char*)output_items[0];

    int consumed = 0;
    int produced = 0;

    // Process input data and create AX.25 frames
    for (int i = 0; i < noutput_items; i++) {
        if (d_frame_length == 0) {
            // Start building a new frame
            build_ax25_frame(in[i]);
        }

        if (d_frame_length > 0) {
            // Output frame data bit by bit
            if (d_bit_position < 8) {
                out[produced] = (d_frame_buffer[d_byte_position] >> (7 - d_bit_position)) & 0x01;
                d_bit_position++;
                produced++;
            } else {
                d_bit_position = 0;
                d_byte_position++;
                if (d_byte_position >= d_frame_length) {
                    // Frame complete, reset for next frame
                    d_frame_length = 0;
                    d_byte_position = 0;
                    d_bit_position = 0;
                    d_frame_buffer.clear();
                }
            }
        }

        consumed++;
    }

    return produced;
}

void ax25_encoder_impl::build_ax25_frame(char data_byte) {
    d_frame_buffer.clear();
    d_frame_length = 0;

    // Create AX.25 frame using the real implementation
    ax25_frame_t frame;
    ax25_address_t dest_addr, src_addr;

    // Set up addresses
    ax25_set_address(&dest_addr, d_dest_callsign.c_str(), std::stoi(d_dest_ssid), true);
    ax25_set_address(&src_addr, d_src_callsign.c_str(), std::stoi(d_src_ssid), false);

    // Create UI frame (for APRS-style transmission)
    uint8_t info[1] = {static_cast<uint8_t>(data_byte)};
    ax25_create_frame(&frame, &src_addr, &dest_addr, AX25_CTRL_UI, AX25_PID_NONE, info, 1);

    // Encode frame
    uint8_t encoded[512];
    uint16_t encoded_len = sizeof(encoded);
    if (ax25_encode_frame(&frame, encoded, &encoded_len) == 0) {
        // Add flags
        if (ax25_add_flags(encoded, &encoded_len, sizeof(encoded)) == 0) {
            // Copy to frame buffer
            for (uint16_t i = 0; i < encoded_len; i++) {
                d_frame_buffer.push_back(encoded[i]);
                d_frame_length++;
            }
        }
    }

    // Reset bit position for output
    d_bit_position = 0;
    d_byte_position = 0;
}

} /* namespace packet_protocols */
} /* namespace gr */

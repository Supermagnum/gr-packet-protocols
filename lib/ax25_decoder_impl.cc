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

#include "ax25_decoder_impl.h"
#include <algorithm>
#include <gnuradio/io_signature.h>

namespace gr {
namespace packet_protocols {

ax25_decoder::sptr ax25_decoder::make() {
    return gnuradio::make_block_sptr<ax25_decoder_impl>();
}

ax25_decoder_impl::ax25_decoder_impl()
    : gr::block("ax25_decoder", gr::io_signature::make(1, 1, sizeof(char)),
                gr::io_signature::make(1, 1, sizeof(char))),
      d_state(STATE_IDLE), d_bit_buffer(0), d_bit_count(0), d_frame_buffer(2048), d_frame_length(0),
      d_ones_count(0), d_escaped(false) {
    d_frame_length = 0;
}

ax25_decoder_impl::~ax25_decoder_impl() {
}

void ax25_decoder_impl::forecast(int noutput_items, gr_vector_int& ninput_items_required)
{
    const int pending = static_cast<int>(d_out_queue.size());
    if (noutput_items <= pending) {
        ninput_items_required[0] = 0;
        return;
    }
    const int deficit = noutput_items - pending;
    ninput_items_required[0] = std::max(deficit * 8, 1);
}

int ax25_decoder_impl::general_work(int noutput_items,
                                    gr_vector_int& ninput_items,
                                    gr_vector_const_void_star& input_items,
                                    gr_vector_void_star& output_items)
{
    const char* in = (const char*)input_items[0];
    char* out = (char*)output_items[0];
    int produced = 0;
    int consumed = 0;
    const int nin = ninput_items[0];

    while (produced < noutput_items && !d_out_queue.empty()) {
        out[produced++] = (char)d_out_queue.front();
        d_out_queue.pop_front();
    }

    while (produced < noutput_items && consumed < nin) {
        bool bit = in[consumed] != 0;
        consumed++;
        process_bit(bit);

        if (d_state == STATE_FRAME_COMPLETE) {
            if (d_frame_length > 0) {
                for (int j = 0; j < d_frame_length; j++)
                    d_out_queue.push_back(d_frame_buffer[j]);
            }
            d_state = STATE_IDLE;
            d_frame_length = 0;
            d_bit_buffer = 0;
            d_bit_count = 0;
            d_ones_count = 0;
            d_escaped = false;

            while (produced < noutput_items && !d_out_queue.empty()) {
                out[produced++] = (char)d_out_queue.front();
                d_out_queue.pop_front();
            }
        }
    }

    consume_each(consumed);
    return produced;
}

void ax25_decoder_impl::process_bit(bool bit)
{
    switch (d_state) {
    case STATE_IDLE:
        if (bit) {
            d_ones_count++;
            if (d_ones_count >= 6) {
                // Found flag sequence, start frame
                d_state = STATE_FLAG;
                d_ones_count = 0;
                d_bit_buffer = 0;
                d_bit_count = 0;
                d_frame_length = 0;
            }
        } else {
            d_ones_count = 0;
        }
        break;

    case STATE_FLAG:
        if (!bit) {
            // End of flag, start data
            d_state = STATE_DATA;
            d_bit_buffer = 0;
            d_bit_count = 0;
            d_ones_count = 0;
        }
        break;

    case STATE_DATA: {
        auto append_data_bit = [&](bool data_bit) {
            d_bit_buffer = static_cast<uint8_t>((d_bit_buffer << 1) | (data_bit ? 1 : 0));
            d_bit_count++;
            if (d_bit_count == 8) {
                uint8_t byte = d_bit_buffer;
                if (d_frame_length < static_cast<int>(d_frame_buffer.size())) {
                    d_frame_buffer[d_frame_length] = byte;
                    d_frame_length++;
                }
                d_bit_buffer = 0;
                d_bit_count = 0;
            }
        };

        if (bit) {
            d_ones_count++;
            if (d_ones_count >= 6) {
                d_state = STATE_FRAME_COMPLETE;
                return;
            }
            append_data_bit(true);
        } else {
            if (d_ones_count == 5) {
                /* Transmitter inserted a 0 after five 1s; drop this stuffed bit */
                d_ones_count = 0;
                return;
            }
            d_ones_count = 0;
            append_data_bit(false);
        }
    } break;

    case STATE_FRAME_COMPLETE:
        // Frame is complete, will be handled in work()
        break;
    }
}

bool ax25_decoder_impl::validate_frame() {
    if (d_frame_length < 18) { // Minimum AX.25 frame size
        return false;
    }

    // Check for valid flags
    if (d_frame_buffer[0] != 0x7E || d_frame_buffer[d_frame_length - 1] != 0x7E) {
        return false;
    }

    // Validate FCS
    uint16_t calculated_fcs = calculate_fcs();
    uint16_t received_fcs =
        d_frame_buffer[d_frame_length - 3] | (d_frame_buffer[d_frame_length - 2] << 8);

    return calculated_fcs == received_fcs;
}

uint16_t ax25_decoder_impl::calculate_fcs() {
    uint16_t fcs = 0xFFFF;

    // Calculate CRC over frame data (excluding flags and FCS)
    for (int i = 1; i < d_frame_length - 3; i++) {
        fcs ^= d_frame_buffer[i];
        for (int j = 0; j < 8; j++) {
            if (fcs & 0x0001) {
                fcs = (fcs >> 1) ^ 0x8408;
            } else {
                fcs = fcs >> 1;
            }
        }
    }

    return fcs ^ 0xFFFF;
}

std::string ax25_decoder_impl::extract_callsign(int start_pos) {
    std::string callsign;

    for (int i = 0; i < 6; i++) {
        char c = (d_frame_buffer[start_pos + i] >> 1) & 0x7F;
        if (c != ' ') {
            callsign += c;
        }
    }

    return callsign;
}

int ax25_decoder_impl::extract_ssid(int pos) {
    return (d_frame_buffer[pos] >> 1) & 0x0F;
}

} /* namespace packet_protocols */
} /* namespace gr */

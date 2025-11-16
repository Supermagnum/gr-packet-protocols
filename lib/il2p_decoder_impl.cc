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

#include "il2p_decoder_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace packet_protocols {

il2p_decoder::sptr il2p_decoder::make() {
    return gnuradio::make_block_sptr<il2p_decoder_impl>();
}

il2p_decoder_impl::il2p_decoder_impl()
    : gr::sync_block("il2p_decoder", gr::io_signature::make(1, 1, sizeof(char)),
                     gr::io_signature::make(1, 1, sizeof(char))),
      d_state(STATE_IDLE), d_bit_buffer(0), d_bit_count(0), d_frame_buffer(2048), d_frame_length(0),
      d_ones_count(0), d_escaped(false), d_fec_type(IL2P_FEC_RS_255_223),
      d_reed_solomon_decoder(nullptr) {
    // Initialize Reed-Solomon decoder
    initialize_reed_solomon();

    d_frame_buffer.clear();
    d_frame_length = 0;
}

il2p_decoder_impl::~il2p_decoder_impl() {
    if (d_reed_solomon_decoder) {
        delete d_reed_solomon_decoder;
    }
}

void il2p_decoder_impl::initialize_reed_solomon() {
    // Initialize Reed-Solomon decoder based on FEC type
    switch (d_fec_type) {
    case IL2P_FEC_RS_255_223:
        d_reed_solomon_decoder = new ReedSolomonDecoder(255, 223);
        break;
    case IL2P_FEC_RS_255_239:
        d_reed_solomon_decoder = new ReedSolomonDecoder(255, 239);
        break;
    case IL2P_FEC_RS_255_247:
        d_reed_solomon_decoder = new ReedSolomonDecoder(255, 247);
        break;
    default:
        d_reed_solomon_decoder = new ReedSolomonDecoder(255, 223);
        break;
    }
}

int il2p_decoder_impl::work(int noutput_items, gr_vector_const_void_star& input_items,
                            gr_vector_void_star& output_items) {
    const char* in = (const char*)input_items[0];
    char* out = (char*)output_items[0];

    int consumed = 0;
    int produced = 0;

    for (int i = 0; i < noutput_items; i++) {
        bool bit = in[i] != 0;

        // Process bit through state machine
        process_bit(bit);

        // Check if we have a complete frame to output
        if (d_state == STATE_FRAME_COMPLETE) {
            if (d_frame_length > 0) {
                // Decode IL2P frame
                std::vector<uint8_t> decoded_data = decode_il2p_frame();

                // Output decoded data
                for (size_t j = 0; j < decoded_data.size() && produced < noutput_items; j++) {
                    out[produced] = decoded_data[j];
                    produced++;
                }
            }

            // Reset for next frame
            d_state = STATE_IDLE;
            d_frame_buffer.clear();
            d_frame_length = 0;
            d_bit_buffer = 0;
            d_bit_count = 0;
            d_ones_count = 0;
            d_escaped = false;
        }

        consumed++;
    }

    return produced;
}

void il2p_decoder_impl::process_bit(bool bit) {
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
                d_frame_buffer.clear();
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

    case STATE_DATA:
        if (bit) {
            d_ones_count++;
            if (d_ones_count >= 6) {
                // Found ending flag
                d_state = STATE_FRAME_COMPLETE;
                return;
            }
        } else {
            d_ones_count = 0;
        }

        // Accumulate bits
        d_bit_buffer = (d_bit_buffer << 1) | (bit ? 1 : 0);
        d_bit_count++;

        if (d_bit_count == 8) {
            // Complete byte received
            uint8_t byte = d_bit_buffer;

            // Handle bit stuffing
            if (d_ones_count == 5 && !d_escaped) {
                // Skip stuffed bit
                d_ones_count = 0;
                d_bit_count = 0;
                d_bit_buffer = 0;
                return;
            }

            // Store byte in frame buffer
            if (d_frame_length < d_frame_buffer.size()) {
                d_frame_buffer[d_frame_length] = byte;
                d_frame_length++;
            }

            // Reset for next byte
            d_bit_buffer = 0;
            d_bit_count = 0;
        }
        break;

    case STATE_FRAME_COMPLETE:
        // Frame is complete, will be handled in work()
        break;
    }
}

std::vector<uint8_t> il2p_decoder_impl::decode_il2p_frame() {
    if (d_frame_length < 8) { // Minimum IL2P frame size
        return std::vector<uint8_t>();
    }

    // Parse IL2P header
    if (!parse_il2p_header()) {
        return std::vector<uint8_t>();
    }

    // Extract data portion
    std::vector<uint8_t> data;
    for (int i = 6; i < d_frame_length - 4; i++) { // Skip header and checksum
        data.push_back(d_frame_buffer[i]);
    }

    // Apply Reed-Solomon decoding
    std::vector<uint8_t> decoded_data = apply_reed_solomon_decode(data);

    return decoded_data;
}

bool il2p_decoder_impl::parse_il2p_header() {
    if (d_frame_length < 6) {
        return false;
    }

    // Check IL2P identifier
    if (d_frame_buffer[1] != 'I' || d_frame_buffer[2] != 'L' || d_frame_buffer[3] != '2' ||
        d_frame_buffer[4] != 'P') {
        return false;
    }

    // Extract FEC type
    d_fec_type = d_frame_buffer[5];

    // Update Reed-Solomon decoder if needed
    initialize_reed_solomon();

    return true;
}

std::vector<uint8_t> il2p_decoder_impl::apply_reed_solomon_decode(
    const std::vector<uint8_t>& data) {
    if (!d_reed_solomon_decoder) {
        return data;
    }

    std::vector<uint8_t> decoded_data;

    // Process data in blocks
    int block_size = d_reed_solomon_decoder->get_code_length();
    int total_blocks = (data.size() + block_size - 1) / block_size;

    for (int block = 0; block < total_blocks; block++) {
        std::vector<uint8_t> block_data;

        // Extract block data
        for (int i = 0; i < block_size; i++) {
            int index = block * block_size + i;
            if (index < data.size()) {
                block_data.push_back(data[index]);
            } else {
                block_data.push_back(0); // Padding
            }
        }

        // Decode block
        std::vector<uint8_t> decoded_block = d_reed_solomon_decoder->decode(block_data);

        // Add to output
        for (size_t i = 0; i < decoded_block.size(); i++) {
            decoded_data.push_back(decoded_block[i]);
        }
    }

    return decoded_data;
}

bool il2p_decoder_impl::validate_checksum() {
    if (d_frame_length < 4) {
        return false;
    }

    uint32_t received_checksum =
        d_frame_buffer[d_frame_length - 4] | (d_frame_buffer[d_frame_length - 3] << 8) |
        (d_frame_buffer[d_frame_length - 2] << 16) | (d_frame_buffer[d_frame_length - 1] << 24);

    uint32_t calculated_checksum = calculate_checksum();

    return calculated_checksum == received_checksum;
}

uint32_t il2p_decoder_impl::calculate_checksum() {
    uint32_t checksum = 0xFFFFFFFF;

    for (int i = 0; i < d_frame_length - 4; i++) {
        checksum ^= d_frame_buffer[i];
        for (int j = 0; j < 8; j++) {
            if (checksum & 0x00000001) {
                checksum = (checksum >> 1) ^ 0xEDB88320;
            } else {
                checksum = checksum >> 1;
            }
        }
    }

    return checksum ^ 0xFFFFFFFF;
}

std::string il2p_decoder_impl::extract_callsign(int start_pos) {
    std::string callsign;

    for (int i = 0; i < 6; i++) {
        char c = (d_frame_buffer[start_pos + i] >> 1) & 0x7F;
        if (c != ' ') {
            callsign += c;
        }
    }

    return callsign;
}

int il2p_decoder_impl::extract_ssid(int pos) {
    return (d_frame_buffer[pos] >> 1) & 0x0F;
}

} /* namespace packet_protocols */
} /* namespace gr */

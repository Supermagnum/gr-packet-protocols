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

#include "fx25_decoder_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace packet_protocols {

fx25_decoder::sptr fx25_decoder::make() {
    return gnuradio::make_block_sptr<fx25_decoder_impl>();
}

fx25_decoder_impl::fx25_decoder_impl()
    : gr::sync_block("fx25_decoder", gr::io_signature::make(1, 1, sizeof(char)),
                     gr::io_signature::make(1, 1, sizeof(char))),
      d_state(STATE_IDLE), d_bit_buffer(0), d_bit_count(0), d_frame_buffer(2048), d_frame_length(0),
      d_ones_count(0), d_escaped(false), d_fec_type(FX25_FEC_RS_16_12), d_interleaver_depth(1),
      d_reed_solomon_decoder(nullptr) {
    // Initialize Reed-Solomon decoder
    initialize_reed_solomon();

    d_frame_buffer.clear();
    d_frame_length = 0;
}

fx25_decoder_impl::~fx25_decoder_impl() {
    if (d_reed_solomon_decoder) {
        delete d_reed_solomon_decoder;
    }
}

void fx25_decoder_impl::initialize_reed_solomon() {
    // Initialize Reed-Solomon decoder based on FEC type
    switch (d_fec_type) {
    case FX25_FEC_RS_12_8:
        d_reed_solomon_decoder = new ReedSolomonDecoder(12, 8);
        break;
    case FX25_FEC_RS_16_12:
        d_reed_solomon_decoder = new ReedSolomonDecoder(16, 12);
        break;
    case FX25_FEC_RS_20_16:
        d_reed_solomon_decoder = new ReedSolomonDecoder(20, 16);
        break;
    case FX25_FEC_RS_24_20:
        d_reed_solomon_decoder = new ReedSolomonDecoder(24, 20);
        break;
    default:
        d_reed_solomon_decoder = new ReedSolomonDecoder(16, 12);
        break;
    }
}

int fx25_decoder_impl::work(int noutput_items, gr_vector_const_void_star& input_items,
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
                // Decode FX.25 frame
                std::vector<uint8_t> decoded_data = decode_fx25_frame();

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

void fx25_decoder_impl::process_bit(bool bit) {
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

std::vector<uint8_t> fx25_decoder_impl::decode_fx25_frame() {
    if (d_frame_length < 8) { // Minimum FX.25 frame size
        return std::vector<uint8_t>();
    }

    // Parse FX.25 header
    if (!parse_fx25_header()) {
        return std::vector<uint8_t>();
    }

    // Extract data portion
    std::vector<uint8_t> data;
    for (int i = 6; i < d_frame_length - 2; i++) { // Skip header and checksum
        data.push_back(d_frame_buffer[i]);
    }

    // Deinterleave data
    std::vector<uint8_t> deinterleaved_data = deinterleave_data(data);

    // Apply Reed-Solomon decoding
    std::vector<uint8_t> decoded_data = apply_reed_solomon_decode(deinterleaved_data);

    return decoded_data;
}

bool fx25_decoder_impl::parse_fx25_header() {
    if (d_frame_length < 6) {
        return false;
    }

    // Check FX.25 identifier
    if (d_frame_buffer[1] != 'F' || d_frame_buffer[2] != 'X' || d_frame_buffer[3] != '2' ||
        d_frame_buffer[4] != '5') {
        return false;
    }

    // Extract FEC type and interleaver depth
    d_fec_type = d_frame_buffer[5];
    d_interleaver_depth = d_frame_buffer[6];

    // Update Reed-Solomon decoder if needed
    initialize_reed_solomon();

    return true;
}

std::vector<uint8_t> fx25_decoder_impl::deinterleave_data(const std::vector<uint8_t>& data) {
    if (d_interleaver_depth <= 1) {
        return data;
    }

    std::vector<uint8_t> deinterleaved_data(data.size());

    for (size_t i = 0; i < data.size(); i++) {
        int original_pos = (i * d_interleaver_depth) % data.size();
        deinterleaved_data[original_pos] = data[i];
    }

    return deinterleaved_data;
}

std::vector<uint8_t> fx25_decoder_impl::apply_reed_solomon_decode(
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

bool fx25_decoder_impl::validate_checksum() {
    if (d_frame_length < 2) {
        return false;
    }

    uint16_t received_checksum =
        d_frame_buffer[d_frame_length - 2] | (d_frame_buffer[d_frame_length - 1] << 8);

    uint16_t calculated_checksum = calculate_checksum();

    return calculated_checksum == received_checksum;
}

uint16_t fx25_decoder_impl::calculate_checksum() {
    uint16_t checksum = 0xFFFF;

    for (int i = 0; i < d_frame_length - 2; i++) {
        checksum ^= d_frame_buffer[i];
        for (int j = 0; j < 8; j++) {
            if (checksum & 0x0001) {
                checksum = (checksum >> 1) ^ 0x8408;
            } else {
                checksum = checksum >> 1;
            }
        }
    }

    return checksum ^ 0xFFFF;
}

} /* namespace packet_protocols */
} /* namespace gr */

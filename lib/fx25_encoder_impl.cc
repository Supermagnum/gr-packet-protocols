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

#include "fx25_encoder_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace packet_protocols {

fx25_encoder::sptr fx25_encoder::make(int fec_type, int interleaver_depth, bool add_checksum) {
    return gnuradio::make_block_sptr<fx25_encoder_impl>(fec_type, interleaver_depth, add_checksum);
}

fx25_encoder_impl::fx25_encoder_impl(int fec_type, int interleaver_depth, bool add_checksum)
    : gr::sync_block("fx25_encoder", gr::io_signature::make(1, 1, sizeof(char)),
                     gr::io_signature::make(1, 1, sizeof(char))),
      d_fec_type(fec_type), d_interleaver_depth(interleaver_depth), d_add_checksum(add_checksum),
      d_frame_buffer(2048), d_frame_length(0), d_bit_position(0), d_byte_position(0),
      d_reed_solomon_encoder(nullptr) {
    // Initialize Reed-Solomon encoder based on FEC type
    initialize_reed_solomon();

    d_frame_buffer.clear();
    d_frame_length = 0;
}

fx25_encoder_impl::~fx25_encoder_impl() {
    if (d_reed_solomon_encoder) {
        delete d_reed_solomon_encoder;
    }
}

void fx25_encoder_impl::initialize_reed_solomon() {
    // Initialize Reed-Solomon encoder based on FEC type
    switch (d_fec_type) {
    case FX25_FEC_RS_12_8:
        d_reed_solomon_encoder = new ReedSolomonEncoder(12, 8);
        break;
    case FX25_FEC_RS_16_12:
        d_reed_solomon_encoder = new ReedSolomonEncoder(16, 12);
        break;
    case FX25_FEC_RS_20_16:
        d_reed_solomon_encoder = new ReedSolomonEncoder(20, 16);
        break;
    case FX25_FEC_RS_24_20:
        d_reed_solomon_encoder = new ReedSolomonEncoder(24, 20);
        break;
    default:
        d_reed_solomon_encoder = new ReedSolomonEncoder(16, 12);
        break;
    }
}

int fx25_encoder_impl::work(int noutput_items, gr_vector_const_void_star& input_items,
                            gr_vector_void_star& output_items) {
    const char* in = (const char*)input_items[0];
    char* out = (char*)output_items[0];

    int consumed = 0;
    int produced = 0;

    // Process input data and create FX.25 frames
    for (int i = 0; i < noutput_items; i++) {
        if (d_frame_length == 0) {
            // Start building a new frame
            build_fx25_frame(in[i]);
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

void fx25_encoder_impl::build_fx25_frame(char data_byte) {
    d_frame_buffer.clear();
    d_frame_length = 0;

    // FX.25 Header
    add_fx25_header();

    // Original AX.25 frame data
    std::vector<uint8_t> ax25_data;
    ax25_data.push_back(data_byte);

    // Add Reed-Solomon FEC
    std::vector<uint8_t> fec_data = apply_reed_solomon_fec(ax25_data);

    // Interleave data
    std::vector<uint8_t> interleaved_data = interleave_data(fec_data);

    // Add to frame buffer
    for (size_t i = 0; i < interleaved_data.size(); i++) {
        d_frame_buffer.push_back(interleaved_data[i]);
        d_frame_length++;
    }

    // Add checksum if requested
    if (d_add_checksum) {
        uint16_t checksum = calculate_checksum();
        d_frame_buffer.push_back(checksum & 0xFF);
        d_frame_buffer.push_back((checksum >> 8) & 0xFF);
        d_frame_length += 2;
    }

    // Reset bit position for output
    d_bit_position = 0;
    d_byte_position = 0;
}

void fx25_encoder_impl::add_fx25_header() {
    // FX.25 frame header
    d_frame_buffer.push_back(0x7E); // Flag
    d_frame_length++;

    // FX.25 identifier
    d_frame_buffer.push_back('F');
    d_frame_buffer.push_back('X');
    d_frame_buffer.push_back('2');
    d_frame_buffer.push_back('5');
    d_frame_length += 4;

    // FEC type
    d_frame_buffer.push_back(d_fec_type & 0xFF);
    d_frame_length++;

    // Interleaver depth
    d_frame_buffer.push_back(d_interleaver_depth & 0xFF);
    d_frame_length++;
}

std::vector<uint8_t> fx25_encoder_impl::apply_reed_solomon_fec(const std::vector<uint8_t>& data) {
    if (!d_reed_solomon_encoder) {
        return data;
    }

    std::vector<uint8_t> encoded_data;

    // Process data in blocks
    int block_size = d_reed_solomon_encoder->get_data_length();
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

        // Encode block
        std::vector<uint8_t> encoded_block = d_reed_solomon_encoder->encode(block_data);

        // Add to output
        for (size_t i = 0; i < encoded_block.size(); i++) {
            encoded_data.push_back(encoded_block[i]);
        }
    }

    return encoded_data;
}

std::vector<uint8_t> fx25_encoder_impl::interleave_data(const std::vector<uint8_t>& data) {
    if (d_interleaver_depth <= 1) {
        return data;
    }

    std::vector<uint8_t> interleaved_data(data.size());

    for (size_t i = 0; i < data.size(); i++) {
        int new_pos = (i * d_interleaver_depth) % data.size();
        interleaved_data[new_pos] = data[i];
    }

    return interleaved_data;
}

uint16_t fx25_encoder_impl::calculate_checksum() {
    uint16_t checksum = 0xFFFF;

    for (int i = 0; i < d_frame_length; i++) {
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

void fx25_encoder_impl::set_fec_type(int fec_type) {
    d_fec_type = fec_type;
    initialize_reed_solomon();
}

void fx25_encoder_impl::set_interleaver_depth(int depth) {
    d_interleaver_depth = depth;
}

void fx25_encoder_impl::set_add_checksum(bool add_checksum) {
    d_add_checksum = add_checksum;
}

} /* namespace packet_protocols */
} /* namespace gr */

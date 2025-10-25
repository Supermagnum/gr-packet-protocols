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

#include "il2p_reed_solomon_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace packet_protocols {

il2p_reed_solomon::sptr il2p_reed_solomon::make(int fec_type, bool encode_mode) {
    return gnuradio::make_block_sptr<il2p_reed_solomon_impl>(fec_type, encode_mode);
}

il2p_reed_solomon_impl::il2p_reed_solomon_impl(int fec_type, bool encode_mode)
    : gr::sync_block("il2p_reed_solomon", gr::io_signature::make(1, 1, sizeof(char)),
                     gr::io_signature::make(1, 1, sizeof(char))),
      d_fec_type(fec_type), d_encode_mode(encode_mode), d_reed_solomon_encoder(nullptr),
      d_reed_solomon_decoder(nullptr), d_frame_buffer(2048), d_frame_length(0), d_bit_position(0),
      d_byte_position(0) {
    // Initialize Reed-Solomon codec
    initialize_reed_solomon();

    d_frame_buffer.clear();
    d_frame_length = 0;
}

il2p_reed_solomon_impl::~il2p_reed_solomon_impl() {
    if (d_reed_solomon_encoder) {
        delete d_reed_solomon_encoder;
    }
    if (d_reed_solomon_decoder) {
        delete d_reed_solomon_decoder;
    }
}

void il2p_reed_solomon_impl::initialize_reed_solomon() {
    // Initialize Reed-Solomon codec based on FEC type
    switch (d_fec_type) {
    case IL2P_FEC_RS_255_223:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 223);
        d_reed_solomon_decoder = new ReedSolomonDecoder(255, 223);
        break;
    case IL2P_FEC_RS_255_239:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 239);
        d_reed_solomon_decoder = new ReedSolomonDecoder(255, 239);
        break;
    case IL2P_FEC_RS_255_247:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 247);
        d_reed_solomon_decoder = new ReedSolomonDecoder(255, 247);
        break;
    default:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 223);
        d_reed_solomon_decoder = new ReedSolomonDecoder(255, 223);
        break;
    }
}

int il2p_reed_solomon_impl::work(int noutput_items, gr_vector_const_void_star& input_items,
                                 gr_vector_void_star& output_items) {
    const char* in = (const char*)input_items[0];
    char* out = (char*)output_items[0];

    int consumed = 0;
    int produced = 0;

    if (d_encode_mode) {
        // Encoding mode - apply Reed-Solomon encoding
        for (int i = 0; i < noutput_items; i++) {
            if (d_frame_length == 0) {
                // Start building a new frame
                build_encoded_frame(in[i]);
            }

            if (d_frame_length > 0) {
                // Output frame data bit by bit
                if (d_bit_position < 8) {
                    out[produced] =
                        (d_frame_buffer[d_byte_position] >> (7 - d_bit_position)) & 0x01;
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
    } else {
        // Decoding mode - apply Reed-Solomon decoding
        for (int i = 0; i < noutput_items; i++) {
            if (d_frame_length == 0) {
                // Start building a new frame
                build_decoded_frame(in[i]);
            }

            if (d_frame_length > 0) {
                // Output frame data bit by bit
                if (d_bit_position < 8) {
                    out[produced] =
                        (d_frame_buffer[d_byte_position] >> (7 - d_bit_position)) & 0x01;
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
    }

    return produced;
}

void il2p_reed_solomon_impl::build_encoded_frame(char data_byte) {
    d_frame_buffer.clear();
    d_frame_length = 0;

    // Original data
    std::vector<uint8_t> original_data;
    original_data.push_back(data_byte);

    // Apply Reed-Solomon encoding
    std::vector<uint8_t> encoded_data = apply_reed_solomon_encode(original_data);

    // Add to frame buffer
    for (size_t i = 0; i < encoded_data.size(); i++) {
        d_frame_buffer.push_back(encoded_data[i]);
        d_frame_length++;
    }

    // Reset bit position for output
    d_bit_position = 0;
    d_byte_position = 0;
}

void il2p_reed_solomon_impl::build_decoded_frame(char data_byte) {
    d_frame_buffer.clear();
    d_frame_length = 0;

    // Received data
    std::vector<uint8_t> received_data;
    received_data.push_back(data_byte);

    // Apply Reed-Solomon decoding
    std::vector<uint8_t> decoded_data = apply_reed_solomon_decode(received_data);

    // Add to frame buffer
    for (size_t i = 0; i < decoded_data.size(); i++) {
        d_frame_buffer.push_back(decoded_data[i]);
        d_frame_length++;
    }

    // Reset bit position for output
    d_bit_position = 0;
    d_byte_position = 0;
}

std::vector<uint8_t> il2p_reed_solomon_impl::apply_reed_solomon_encode(
    const std::vector<uint8_t>& data) {
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

std::vector<uint8_t> il2p_reed_solomon_impl::apply_reed_solomon_decode(
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

void il2p_reed_solomon_impl::set_fec_type(int fec_type) {
    d_fec_type = fec_type;
    initialize_reed_solomon();
}

void il2p_reed_solomon_impl::set_encode_mode(bool encode_mode) {
    d_encode_mode = encode_mode;
}

int il2p_reed_solomon_impl::get_data_length() const {
    if (d_reed_solomon_encoder) {
        return d_reed_solomon_encoder->get_data_length();
    }
    return 0;
}

int il2p_reed_solomon_impl::get_code_length() const {
    if (d_reed_solomon_encoder) {
        return d_reed_solomon_encoder->get_code_length();
    }
    return 0;
}

int il2p_reed_solomon_impl::get_error_correction_capability() const {
    if (d_reed_solomon_encoder) {
        return d_reed_solomon_encoder->get_error_correction_capability();
    }
    return 0;
}

} /* namespace packet_protocols */
} /* namespace gr */

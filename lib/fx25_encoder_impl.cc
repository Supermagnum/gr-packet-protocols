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

namespace {

void push_msb_bits_raw(uint8_t byte, std::vector<uint8_t>& q)
{
    for (int bp = 7; bp >= 0; --bp)
        q.push_back(static_cast<uint8_t>((byte >> bp) & 1));
}

void push_msb_bits_stuffed(uint8_t byte, std::vector<uint8_t>& q, int& ones_run)
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

} // namespace

fx25_encoder::sptr fx25_encoder::make(int fec_type, int interleaver_depth, bool add_checksum) {
    return gnuradio::make_block_sptr<fx25_encoder_impl>(fec_type, interleaver_depth, add_checksum);
}

fx25_encoder_impl::fx25_encoder_impl(int fec_type, int interleaver_depth, bool add_checksum)
    : gr::block("fx25_encoder", gr::io_signature::make(1, 1, sizeof(char)),
                gr::io_signature::make(1, 1, sizeof(char))),
      d_fec_type(fec_type), d_interleaver_depth(interleaver_depth), d_add_checksum(add_checksum),
      d_frame_buffer(), d_frame_length(0), d_bit_queue(), d_bit_q_read(0),
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
    // Initialize Reed-Solomon encoder based on FEC type (FX.25 uses RS(255,k) codes)
    switch (d_fec_type) {
    case FX25_FEC_RS_255_239:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 239);
        break;
    case FX25_FEC_RS_255_223:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 223);
        break;
    case FX25_FEC_RS_255_191:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 191);
        break;
    case FX25_FEC_RS_255_159:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 159);
        break;
    case FX25_FEC_RS_255_127:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 127);
        break;
    case FX25_FEC_RS_255_95:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 95);
        break;
    case FX25_FEC_RS_255_63:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 63);
        break;
    case FX25_FEC_RS_255_31:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 31);
        break;
    default:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 223); // Default: 32 parity bytes
        break;
    }
}

void fx25_encoder_impl::forecast(int noutput_items, gr_vector_int& ninput_items_required)
{
    if (d_bit_q_read < d_bit_queue.size()) {
        ninput_items_required[0] = 0;
        return;
    }
    ninput_items_required[0] = (noutput_items > 0) ? 1 : 0;
}

int fx25_encoder_impl::general_work(int noutput_items,
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
        build_fx25_frame(in[consumed]);
        consumed++;
    }

    if (d_bit_q_read >= d_bit_queue.size() && !d_bit_queue.empty()) {
        d_bit_queue.clear();
        d_bit_q_read = 0;
    }

    consume_each(consumed);
    return produced;
}

void fx25_encoder_impl::rebuild_bit_queue_from_frame()
{
    d_bit_queue.clear();
    d_bit_q_read = 0;
    if (d_frame_length < 2)
        return;
    /* Raw HDLC flags MSB-first; stuff only octets between opening and closing delimiters */
    push_msb_bits_raw(d_frame_buffer[0], d_bit_queue);
    int ones_run = 0;
    for (uint16_t i = 1; i + 1 < d_frame_length; ++i)
        push_msb_bits_stuffed(d_frame_buffer[i], d_bit_queue, ones_run);
    push_msb_bits_raw(d_frame_buffer[d_frame_length - 1], d_bit_queue);
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

    /* Closing HDLC flag so the decoder can exit STATE_DATA */
    d_frame_buffer.push_back(0x7E);
    d_frame_length++;

    rebuild_bit_queue_from_frame();
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

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

#include "il2p_encoder_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace packet_protocols {

il2p_encoder::sptr il2p_encoder::make(const std::string& dest_callsign,
                                      const std::string& dest_ssid, const std::string& src_callsign,
                                      const std::string& src_ssid, int fec_type,
                                      bool add_checksum) {
    return gnuradio::make_block_sptr<il2p_encoder_impl>(dest_callsign, dest_ssid, src_callsign,
                                                        src_ssid, fec_type, add_checksum);
}

il2p_encoder_impl::il2p_encoder_impl(const std::string& dest_callsign, const std::string& dest_ssid,
                                     const std::string& src_callsign, const std::string& src_ssid,
                                     int fec_type, bool add_checksum)
    : gr::block("il2p_encoder", gr::io_signature::make(1, 1, sizeof(char)),
                gr::io_signature::make(1, 1, sizeof(char))),
      d_dest_callsign(dest_callsign), d_dest_ssid(dest_ssid), d_src_callsign(src_callsign),
      d_src_ssid(src_ssid), d_fec_type(fec_type), d_add_checksum(add_checksum), d_frame_length(0),
      d_bit_q_read(0), d_reed_solomon_encoder(nullptr) {
    // Initialize Reed-Solomon encoder
    initialize_reed_solomon();

    d_frame_buffer.clear();
    d_frame_length = 0;
}

il2p_encoder_impl::~il2p_encoder_impl() {
    if (d_reed_solomon_encoder) {
        delete d_reed_solomon_encoder;
    }
}

void il2p_encoder_impl::initialize_reed_solomon() {
    // Initialize Reed-Solomon encoder based on FEC type
    switch (d_fec_type) {
    case IL2P_FEC_RS_255_223:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 223);
        break;
    case IL2P_FEC_RS_255_239:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 239);
        break;
    case IL2P_FEC_RS_255_247:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 247);
        break;
    default:
        d_reed_solomon_encoder = new ReedSolomonEncoder(255, 223);
        break;
    }
}

void il2p_encoder_impl::push_msb_bits_raw(uint8_t byte, std::vector<uint8_t>& q)
{
    for (int bp = 7; bp >= 0; --bp)
        q.push_back(static_cast<uint8_t>((byte >> bp) & 1));
}

void il2p_encoder_impl::push_msb_bits_stuffed(uint8_t byte, std::vector<uint8_t>& q, int& ones_run)
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

void il2p_encoder_impl::rebuild_bit_queue_from_frame()
{
    d_bit_queue.clear();
    d_bit_q_read = 0;
    if (d_frame_buffer.size() < 2)
        return;
    push_msb_bits_raw(d_frame_buffer[0], d_bit_queue);
    int ones_run = 0;
    for (size_t i = 1; i + 1 < d_frame_buffer.size(); ++i)
        push_msb_bits_stuffed(d_frame_buffer[i], d_bit_queue, ones_run);
    push_msb_bits_raw(d_frame_buffer[d_frame_buffer.size() - 1], d_bit_queue);
}

void il2p_encoder_impl::forecast(int noutput_items, gr_vector_int& ninput_items_required)
{
    if (d_bit_q_read < d_bit_queue.size()) {
        ninput_items_required[0] = 0;
        return;
    }
    ninput_items_required[0] = (noutput_items > 0) ? 1 : 0;
}

int il2p_encoder_impl::general_work(int noutput_items,
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
        build_il2p_frame(in[consumed]);
        consumed++;
    }

    if (d_bit_q_read >= d_bit_queue.size() && !d_bit_queue.empty()) {
        d_bit_queue.clear();
        d_bit_q_read = 0;
    }

    consume_each(consumed);
    return produced;
}

void il2p_encoder_impl::build_il2p_frame(char data_byte) {
    d_frame_buffer.clear();
    d_frame_length = 0;

    // IL2P Header
    add_il2p_header();

    // Original data
    std::vector<uint8_t> original_data;
    original_data.push_back(data_byte);

    // Apply Reed-Solomon FEC
    std::vector<uint8_t> fec_data = apply_reed_solomon_fec(original_data);

    // Scramble data (IL2P uses scrambling to improve bit transitions)
    std::vector<uint8_t> scrambled_data = scramble_data(fec_data);

    // Add to frame buffer
    for (size_t i = 0; i < scrambled_data.size(); i++) {
        d_frame_buffer.push_back(scrambled_data[i]);
        d_frame_length++;
    }

    // Add checksum if requested
    if (d_add_checksum) {
        uint32_t checksum = calculate_checksum();
        d_frame_buffer.push_back(checksum & 0xFF);
        d_frame_buffer.push_back((checksum >> 8) & 0xFF);
        d_frame_buffer.push_back((checksum >> 16) & 0xFF);
        d_frame_buffer.push_back((checksum >> 24) & 0xFF);
        d_frame_length += 4;
    }

    /* HDLC framing: checksum covers interior only; raw flags on the wire */
    d_frame_buffer.insert(d_frame_buffer.begin(), static_cast<uint8_t>(0x7E));
    d_frame_buffer.push_back(static_cast<uint8_t>(0x7E));

    rebuild_bit_queue_from_frame();
}

void il2p_encoder_impl::add_il2p_header() {
    // IL2P preamble (0x55)
    d_frame_buffer.push_back(IL2P_PREAMBLE);
    d_frame_length++;

    // IL2P sync word (0xF15E48)
    d_frame_buffer.push_back(0xF1);
    d_frame_buffer.push_back(0x5E);
    d_frame_buffer.push_back(0x48);
    d_frame_length += 3;

    // FEC type
    d_frame_buffer.push_back(d_fec_type & 0xFF);
    d_frame_length++;

    // Destination address
    add_address(d_dest_callsign, d_dest_ssid, false);

    // Source address
    add_address(d_src_callsign, d_src_ssid, true);
}

void il2p_encoder_impl::add_address(const std::string& callsign, const std::string& ssid,
                                    bool is_last) {
    // Convert callsign to IL2P address format
    for (size_t i = 0; i < 6; i++) {
        if (i < callsign.length()) {
            d_frame_buffer.push_back(callsign[i] << 1);
        } else {
            d_frame_buffer.push_back(0x20 << 1); // Space padding
        }
        d_frame_length++;
    }

    // SSID field
    int ssid_val = std::stoi(ssid);
    uint8_t ssid_byte = (ssid_val & 0x0F) << 1;
    if (is_last) {
        ssid_byte |= 0x01; // Command/Response bit
    }
    d_frame_buffer.push_back(ssid_byte);
    d_frame_length++;
}

std::vector<uint8_t> il2p_encoder_impl::apply_reed_solomon_fec(const std::vector<uint8_t>& data) {
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

uint32_t il2p_encoder_impl::calculate_checksum() {
    uint32_t checksum = 0xFFFFFFFF;

    for (int i = 0; i < d_frame_length; i++) {
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

void il2p_encoder_impl::set_fec_type(int fec_type) {
    d_fec_type = fec_type;
    initialize_reed_solomon();
}

void il2p_encoder_impl::set_add_checksum(bool add_checksum) {
    d_add_checksum = add_checksum;
}

std::vector<uint8_t> il2p_encoder_impl::scramble_data(const std::vector<uint8_t>& data) {
    // IL2P uses a simple XOR scrambler with polynomial 0x21 (x^5 + x^0)
    // This improves bit transitions for better clock recovery
    std::vector<uint8_t> scrambled(data.size());
    uint8_t scrambler_state = 0x1F; // Initial state (all ones)
    
    for (size_t i = 0; i < data.size(); i++) {
        uint8_t output_bit = 0;
        for (int bit = 0; bit < 8; bit++) {
            // XOR feedback
            uint8_t feedback = ((scrambler_state >> 4) ^ (scrambler_state >> 0)) & 0x01;
            scrambler_state = ((scrambler_state << 1) | feedback) & 0x1F;
            
            // XOR with input bit
            uint8_t input_bit = (data[i] >> (7 - bit)) & 0x01;
            output_bit |= ((input_bit ^ feedback) << (7 - bit));
        }
        scrambled[i] = output_bit;
    }
    
    return scrambled;
}

} /* namespace packet_protocols */
} /* namespace gr */

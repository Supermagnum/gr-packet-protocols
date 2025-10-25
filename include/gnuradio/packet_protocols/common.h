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

#ifndef INCLUDED_PACKET_PROTOCOLS_COMMON_H
#define INCLUDED_PACKET_PROTOCOLS_COMMON_H

#include <vector>
#include <cstdint>

// AX.25 Constants
#define AX25_FLAG 0x7E
#define AX25_FRAME_MIN_SIZE 18

// KISS TNC Constants
#define KISS_FEND 0xC0
#define KISS_FESC 0xDB
#define KISS_TFEND 0xDC
#define KISS_TFESC 0xDD

#define KISS_CMD_DATA 0x00
#define KISS_CMD_TXDELAY 0x01
#define KISS_CMD_P 0x02
#define KISS_CMD_SLOTTIME 0x03
#define KISS_CMD_TXTAIL 0x04
#define KISS_CMD_FULLDUPLEX 0x05
#define KISS_CMD_SET_HARDWARE 0x06
#define KISS_CMD_RETURN 0xFF

// FX.25 FEC Types
#define FX25_FEC_RS_12_8 0x01
#define FX25_FEC_RS_16_12 0x02
#define FX25_FEC_RS_20_16 0x03
#define FX25_FEC_RS_24_20 0x04

// IL2P FEC Types
#define IL2P_FEC_RS_255_223 0x01
#define IL2P_FEC_RS_255_239 0x02
#define IL2P_FEC_RS_255_247 0x03

// Reed-Solomon Codec Classes
class ReedSolomonEncoder {
  public:
    ReedSolomonEncoder(int n, int k) : d_n(n), d_k(k) {}
    ~ReedSolomonEncoder() = default;

    std::vector<uint8_t> encode(const std::vector<uint8_t>& data) {
        // Simple pass-through implementation for now
        // In a real implementation, this would perform Reed-Solomon encoding
        std::vector<uint8_t> result = data;
        // Add parity bytes (simplified)
        for (int i = 0; i < (d_n - d_k); ++i) {
            result.push_back(0);
        }
        return result;
    }
    
    int get_data_length() const {
        return d_k;
    }
    int get_code_length() const {
        return d_n;
    }
    int get_error_correction_capability() const {
        return (d_n - d_k) / 2;
    }

  private:
    int d_n, d_k;
};

class ReedSolomonDecoder {
  public:
    ReedSolomonDecoder(int n, int k) : d_n(n), d_k(k) {}
    ~ReedSolomonDecoder() = default;

    std::vector<uint8_t> decode(const std::vector<uint8_t>& data) {
        // Simple pass-through implementation for now
        // In a real implementation, this would perform Reed-Solomon decoding
        std::vector<uint8_t> result;
        // Return only the data portion (remove parity bytes)
        for (int i = 0; i < d_k && i < (int)data.size(); ++i) {
            result.push_back(data[i]);
        }
        return result;
    }
    
    int get_code_length() const {
        return d_n;
    }
    int get_data_length() const {
        return d_k;
    }

  private:
    int d_n, d_k;
};

#endif /* INCLUDED_PACKET_PROTOCOLS_COMMON_H */

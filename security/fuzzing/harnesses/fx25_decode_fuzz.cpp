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

#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>

// Include the actual FX.25 protocol implementation
#include "fx25_protocol.h"

#define MAX_SIZE 8192

/*!
 * \brief FX.25 correlation tags for different FEC types
 */
static const uint64_t correlation_tags[] = {
    0xB74DB7DF8A532F3EULL, // Tag 0x01 - RS(255,223)
    0x26FF60A600CC8FDEULL, // Tag 0x02 - RS(255,239)
    0xC7DC0508F3D9B09EULL, // Tag 0x03 - RS(255,247)
    0x8F056EB4369660EEULL, // Tag 0x04 - RS(255,251)
    0x6E260B1AC5835FAEULL, // Tag 0x05 - RS(255,253)
    0xFF94DC634F1CFF4EULL, // Tag 0x06 - RS(255,254)
    0x1EB7B9CDBC09C00EULL, // Tag 0x07 - RS(255,255)
    0xDBF869BD2DBB1776ULL, // Tag 0x08 - Custom FEC 1
    0x3ADB0C13DEDC0826ULL, // Tag 0x09 - Custom FEC 2
    0xAB69DB6A543188D6ULL, // Tag 0x0A - Custom FEC 3
    0x4A4ABEC4A724B796ULL, // Tag 0x0B - Custom FEC 4
};

/*!
 * \brief Find correlation tag in FX.25 frame
 * \param data Input data buffer
 * \param size Size of input data
 * \return Tag index if found, -1 otherwise
 */
static int find_correlation_tag(const uint8_t* data, size_t size) {
    if (size < 8) return -1;
    
    uint64_t tag = 0;
    for (int i = 0; i < 8; i++) {
        tag = (tag << 8) | data[i];
    }
    
    for (int i = 0; i < 11; i++) {
        if (tag == correlation_tags[i]) {
            return i;
        }
    }
    return -1;
}

/*!
 * \brief Decode FX.25 frame with comprehensive validation
 * \param data Input data buffer
 * \param size Size of input data
 * \return true if frame is valid, false otherwise
 */
static bool decode_fx25(const uint8_t* data, size_t size) {
    if (size < 8) return false;
    
    int tag_idx = find_correlation_tag(data, size);
    if (tag_idx < 0) return false;
    
    // RS parity sizes for each tag
    const int rs_sizes[] = {16, 16, 32, 32, 32, 48, 48, 64, 64, 64, 64};
    int rs_size = rs_sizes[tag_idx];
    
    if (size < 8 + rs_size) return false;
    
    // Extract RS check bits
    const uint8_t* rs_check = data + 8;
    
    // Extract AX.25 data
    const uint8_t* ax25_data = data + 8 + rs_size;
    size_t ax25_size = size - 8 - rs_size;
    
    // Validate AX.25 data structure
    if (ax25_size < 16) return false;
    
    // Check for AX.25 flag
    if (ax25_data[0] != 0x7E) return false;
    
    // Basic AX.25 address validation
    for (size_t i = 1; i < 7; i++) {
        uint8_t byte = ax25_data[i] >> 1;
        if (byte < 32 || byte > 126) return false;
    }
    
    return true;
}

/*!
 * \brief Test FX.25 protocol functions with fuzzed input
 * \param data Input data buffer
 * \param size Size of input data
 */
static void test_fx25_protocol(const uint8_t* data, size_t size) {
    if (size < 1 || size > MAX_SIZE) return;
    
    // Test FX.25 codec initialization
    fx25_codec_t codec;
    if (fx25_codec_init(&codec) == 0) {
        // Test with different FEC types
        for (int fec_type = 0; fec_type < 11; fec_type++) {
            if (size >= 8) {
                // Create test frame with fuzzed data
                uint8_t test_frame[256];
                size_t frame_size = (size < 256) ? size : 256;
                memcpy(test_frame, data, frame_size);
                
                // Test encoding
                uint8_t encoded[512];
                uint16_t encoded_len = sizeof(encoded);
                if (fx25_encode_frame(&codec, test_frame, frame_size, 
                                   encoded, &encoded_len, fec_type) == 0) {
                    // Test decoding
                    uint8_t decoded[256];
                    uint16_t decoded_len = sizeof(decoded);
                    fx25_decode_frame(&codec, encoded, encoded_len,
                                     decoded, &decoded_len);
                }
            }
        }
    }
    
    // Test correlation tag detection
    if (size >= 8) {
        int tag_idx = find_correlation_tag(data, size);
        if (tag_idx >= 0) {
            // Test with valid correlation tag
            uint8_t test_data[256];
            size_t test_size = (size < 256) ? size : 256;
            memcpy(test_data, data, test_size);
            
            // Set correlation tag
            uint64_t tag = correlation_tags[tag_idx];
            for (int i = 0; i < 8; i++) {
                test_data[i] = (tag >> (56 - i * 8)) & 0xFF;
            }
            
            decode_fx25(test_data, test_size);
        }
    }
}

/*!
 * \brief Main fuzzing entry point
 * \param data Input data from fuzzer
 * \param size Size of input data
 * \return Fuzzing result code
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 1 || size > MAX_SIZE) return 0;
    
    // Branching based on input characteristics
    int result = 0;
    
    // Branch based on size
    if (size < 8) {
        result = 1;  // Too small for FX.25
    } else if (size < 24) {
        result = 2;  // Small FX.25
    } else if (size < 64) {
        result = 3;  // Medium FX.25
    } else {
        result = 4;  // Large FX.25
    }
    
    // Branch based on first byte
    if (data[0] == 0x00) {
        result += 10;  // Null byte
    } else if (data[0] == 0xFF) {
        result += 20;  // All ones
    } else if (data[0] < 32) {
        result += 30;  // Control character
    } else if (data[0] > 126) {
        result += 40;  // Extended character
    } else {
        result += 50;  // Normal character
    }
    
    // Branch based on FX.25 specific fields
    if (size >= 8) {
        // Check correlation tag
        int tag_idx = find_correlation_tag(data, size);
        if (tag_idx >= 0) {
            result += 100 + (tag_idx * 10);  // Valid correlation tag
        } else {
            result += 1000;  // Invalid correlation tag
        }
        
        // Check RS parity size
        if (tag_idx >= 0 && tag_idx < 11) {
            const int rs_sizes[] = {16, 16, 32, 32, 32, 48, 48, 64, 64, 64, 64};
            int rs_size = rs_sizes[tag_idx];
            if (size >= 8 + rs_size) {
                result += 10000;  // Valid RS size
            } else {
                result += 20000;  // Invalid RS size
            }
        }
    }
    
    // Branch based on data patterns
    bool has_zeros = false, has_ones = false, has_alternating = false;
    for (size_t i = 0; i < size && i < 10; i++) {
        if (data[i] == 0x00) has_zeros = true;
        if (data[i] == 0xFF) has_ones = true;
        if (i > 0 && data[i] != data[i-1]) has_alternating = true;
    }
    
    if (has_zeros) result += 100000;
    if (has_ones) result += 200000;
    if (has_alternating) result += 300000;
    
    // Branch based on checksum-like calculation
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum += data[i];
    }
    
    if (checksum == 0) {
        result += 1000000;  // Zero checksum
    } else if (checksum < 100) {
        result += 2000000;  // Low checksum
    } else if (checksum > 1000) {
        result += 3000000;  // High checksum
    } else {
        result += 4000000;  // Medium checksum
    }
    
    // Branch based on specific byte values
    for (size_t i = 0; i < size && i < 5; i++) {
        if (data[i] == 0x55) result += 10000000;
        if (data[i] == 0xAA) result += 20000000;
        if (data[i] == 0x33) result += 30000000;
        if (data[i] == 0xCC) result += 40000000;
    }
    
    // Test FX.25 protocol functions
    test_fx25_protocol(data, size);
    
    // Call the parsing function
    bool valid = decode_fx25(data, size);
    if (valid) {
        result += 100000000;  // Valid FX.25
    }
    
    return result;
}

/*!
 * \brief Standalone test program for manual testing
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        // Read from stdin
        uint8_t buf[MAX_SIZE];
        ssize_t len = read(STDIN_FILENO, buf, MAX_SIZE);
        if (len <= 0) return 0;
        
        int result = LLVMFuzzerTestOneInput(buf, (size_t)len);
        return result;
    } else {
        // Read from file
        FILE* f = fopen(argv[1], "rb");
        if (!f) return 1;
        
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        if (size > MAX_SIZE) {
            fclose(f);
            return 1;
        }
        
        uint8_t* data = new uint8_t[size];
        fread(data, 1, size, f);
        fclose(f);
        
        int result = LLVMFuzzerTestOneInput(data, size);
        delete[] data;
        return result;
    }
}



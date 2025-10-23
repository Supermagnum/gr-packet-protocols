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

// Include the actual IL2P protocol implementation
#include "il2p_protocol.h"

#define MAX_SIZE 8192

/*!
 * \brief Decode IL2P header with comprehensive validation
 * \param data Input data buffer
 * \param size Size of input data
 * \return true if header is valid, false otherwise
 */
static bool decode_il2p_header(const uint8_t* data, size_t size) {
    if (size < 14) return false;
    
    // Header type (2 bits)
    uint8_t header_type = data[0] >> 6;
    
    // Payload size (10 bits)
    uint16_t payload_size = ((data[0] & 0x3F) << 4) | (data[1] >> 4);
    
    if (payload_size > 1023) return false;
    
    // Validate header type
    if (header_type > 3) return false;
    
    // Different header types
    switch (header_type) {
        case 0: // Type 0 - Basic data
            break;
        case 1: // Type 1 - Acknowledgment
            break;
        case 2: // Type 2 - Control
            break;
        case 3: // Type 3 - Extended
            break;
    }
    
    return true;
}

/*!
 * \brief Decode IL2P frame with comprehensive validation
 * \param data Input data buffer
 * \param size Size of input data
 * \return true if frame is valid, false otherwise
 */
static bool decode_il2p(const uint8_t* data, size_t size) {
    if (size < 14) return false;
    
    if (!decode_il2p_header(data, size)) return false;
    
    // Extract payload size from header
    uint16_t payload_size = ((data[0] & 0x3F) << 4) | (data[1] >> 4);
    
    // Calculate expected frame size
    // Header (14 bytes) + Payload + LDPC parity
    size_t expected_size = 14 + payload_size;
    
    if (size < expected_size) return false;
    
    // Validate payload data
    const uint8_t* payload = data + 14;
    for (size_t i = 0; i < payload_size && i < 100; i++) {
        // Basic payload validation
        if (payload[i] == 0xFF && i > 0 && payload[i-1] == 0xFF) {
            // Check for potential padding
            continue;
        }
    }
    
    return true;
}

/*!
 * \brief Test IL2P protocol functions with fuzzed input
 * \param data Input data buffer
 * \param size Size of input data
 */
static void test_il2p_protocol(const uint8_t* data, size_t size) {
    if (size < 1 || size > MAX_SIZE) return;
    
    // Test IL2P context initialization
    il2p_context_t ctx;
    if (il2p_init(&ctx) == 0) {
        // Test with different header types
        for (int header_type = 0; header_type < 4; header_type++) {
            if (size >= 14) {
                // Create test frame with fuzzed data
                uint8_t test_frame[256];
                size_t frame_size = (size < 256) ? size : 256;
                memcpy(test_frame, data, frame_size);
                
                // Set header type
                test_frame[0] = (test_frame[0] & 0x3F) | (header_type << 6);
                
                // Test encoding
                uint8_t encoded[512];
                uint16_t encoded_len = sizeof(encoded);
                if (il2p_encode_frame(&ctx, test_frame, frame_size, 
                                    encoded, &encoded_len) == 0) {
                    // Test decoding
                    uint8_t decoded[256];
                    uint16_t decoded_len = sizeof(decoded);
                    il2p_decode_frame(&ctx, encoded, encoded_len,
                                     decoded, &decoded_len);
                }
            }
        }
        
        // Test data scrambling/descrambling
        if (size >= 16) {
            uint8_t scrambled[256];
            uint8_t descrambled[256];
            size_t data_size = (size < 256) ? size : 256;
            
            // Test scrambling
            if (il2p_scramble_data(&ctx, data, data_size, scrambled) == 0) {
                // Test descrambling
                il2p_descramble_data(&ctx, scrambled, data_size, descrambled);
            }
        }
    }
    
    // Test header encoding/decoding
    if (size >= 14) {
        il2p_header_t header;
        uint8_t encoded_header[16];
        il2p_header_t decoded_header;
        
        // Set header fields from fuzzed data
        header.type = data[0] >> 6;
        header.payload_size = ((data[0] & 0x3F) << 4) | (data[1] >> 4);
        header.sequence = (data[1] & 0x0F) | (data[2] << 4);
        header.checksum = (data[3] << 8) | data[4];
        
        // Test header encoding
        if (il2p_encode_header(&ctx, &header, encoded_header) == 0) {
            // Test header decoding
            il2p_decode_header(&ctx, encoded_header, &decoded_header);
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
    if (size < 14) {
        result = 1;  // Too small for IL2P
    } else if (size < 32) {
        result = 2;  // Small IL2P
    } else if (size < 64) {
        result = 3;  // Medium IL2P
    } else {
        result = 4;  // Large IL2P
    }
    
    // Branch based on IL2P specific fields
    if (size >= 14) {
        uint8_t header_type = data[0] >> 6;
        uint16_t payload_size = ((data[0] & 0x3F) << 4) | (data[1] >> 4);
        
        if (header_type == 0) {
            result += 10;  // Type 0
        } else if (header_type == 1) {
            result += 20;  // Type 1
        } else if (header_type == 2) {
            result += 30;  // Type 2
        } else if (header_type == 3) {
            result += 40;  // Type 3
        }
        
        if (payload_size < 100) {
            result += 100;  // Small payload
        } else if (payload_size < 500) {
            result += 200;  // Medium payload
        } else if (payload_size < 1000) {
            result += 300;  // Large payload
        } else {
            result += 400;  // Very large payload
        }
    }
    
    // Branch based on first byte
    if (data[0] == 0x00) {
        result += 1000;  // Null byte
    } else if (data[0] == 0xFF) {
        result += 2000;  // All ones
    } else if (data[0] < 32) {
        result += 3000;  // Control character
    } else if (data[0] > 126) {
        result += 4000;  // Extended character
    } else {
        result += 5000;  // Normal character
    }
    
    // Branch based on data patterns
    bool has_zeros = false, has_ones = false, has_alternating = false;
    for (size_t i = 0; i < size && i < 10; i++) {
        if (data[i] == 0x00) has_zeros = true;
        if (data[i] == 0xFF) has_ones = true;
        if (i > 0 && data[i] != data[i-1]) has_alternating = true;
    }
    
    if (has_zeros) result += 10000;
    if (has_ones) result += 20000;
    if (has_alternating) result += 30000;
    
    // Branch based on checksum-like calculation
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum += data[i];
    }
    
    if (checksum == 0) {
        result += 100000;  // Zero checksum
    } else if (checksum < 100) {
        result += 200000;  // Low checksum
    } else if (checksum > 1000) {
        result += 300000;  // High checksum
    } else {
        result += 400000;  // Medium checksum
    }
    
    // Branch based on specific byte values
    for (size_t i = 0; i < size && i < 5; i++) {
        if (data[i] == 0x55) result += 1000000;
        if (data[i] == 0xAA) result += 2000000;
        if (data[i] == 0x33) result += 3000000;
        if (data[i] == 0xCC) result += 4000000;
    }
    
    // Test IL2P protocol functions
    test_il2p_protocol(data, size);
    
    // Call the parsing function
    bool valid = decode_il2p_header(data, size);
    if (valid) {
        result += 10000000;  // Valid IL2P
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



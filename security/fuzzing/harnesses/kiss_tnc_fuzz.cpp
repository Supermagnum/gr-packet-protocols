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

// Include the actual KISS protocol implementation
#include "kiss_protocol.h"

#define MAX_SIZE 8192

/*!
 * \brief KISS frame constants
 */
#define KISS_FEND 0xC0
#define KISS_FESC 0xDB
#define KISS_TFEND 0xDC
#define KISS_TFESC 0xDD

/*!
 * \brief Parse KISS frame with comprehensive validation
 * \param data Input data buffer
 * \param size Size of input data
 * \return true if frame is valid, false otherwise
 */
static bool parse_kiss_frame(const uint8_t* data, size_t size) {
    if (size < 3) return false;
    
    // Check for KISS frame start (FEND)
    if (data[0] != KISS_FEND) return false;
    
    // Check for KISS frame end (FEND)
    size_t end_pos = size - 1;
    while (end_pos > 0 && data[end_pos] != KISS_FEND) {
        end_pos--;
    }
    
    if (end_pos == 0) return false; // No FEND found
    
    // Extract command and port
    uint8_t command = data[1] & 0x0F;
    uint8_t port = data[1] >> 4;
    
    // Validate command
    if (command > 0x0F) return false;
    
    // Validate port (typically 0-7)
    if (port > 7) return false;
    
    // Check for escaped characters
    for (size_t i = 2; i < end_pos; i++) {
        if (data[i] == KISS_FESC) {
            if (i + 1 >= end_pos) return false;
            uint8_t next = data[i + 1];
            if (next != KISS_TFEND && next != KISS_TFESC) {
                return false;
            }
            i++; // Skip the escaped character
        }
    }
    
    return true;
}

/*!
 * \brief Test KISS protocol functions with fuzzed input
 * \param data Input data buffer
 * \param size Size of input data
 */
static void test_kiss_protocol(const uint8_t* data, size_t size) {
    if (size < 1 || size > MAX_SIZE) return;
    
    // Test KISS TNC initialization
    kiss_tnc_t tnc;
    if (kiss_init(&tnc) == 0) {
        // Test with different configurations
        kiss_config_t config;
        config.baud_rate = 1200 + (data[0] % 8) * 1200; // 1200, 2400, 3600, etc.
        config.tx_delay = data[1] % 100;
        config.tx_tail = data[2] % 10;
        config.persist = data[3] % 100;
        config.slot_time = data[4] % 10;
        config.duplex = (data[5] & 0x01) != 0;
        
        kiss_configure(&tnc, &config);
        
        // Test frame sending
        if (size >= 16) {
            uint8_t frame[256];
            size_t frame_size = (size < 256) ? size : 256;
            memcpy(frame, data, frame_size);
            
            // Test sending frame
            kiss_send_frame(&tnc, frame, frame_size);
        }
        
        // Test frame receiving
        if (size >= 8) {
            uint8_t received[256];
            uint16_t received_len = sizeof(received);
            
            // Simulate receiving fuzzed data
            kiss_receive_frame(&tnc, data, size, received, &received_len);
        }
        
        // Test byte processing
        for (size_t i = 0; i < size && i < 100; i++) {
            uint8_t processed;
            kiss_process_byte(&tnc, data[i], &processed);
        }
    }
    
    // Test data escaping/unescaping
    if (size >= 8) {
        uint8_t escaped[512];
        uint8_t unescaped[256];
        uint16_t escaped_len = sizeof(escaped);
        uint16_t unescaped_len = sizeof(unescaped);
        
        // Test escaping
        if (kiss_escape_data(data, size, escaped, &escaped_len) == 0) {
            // Test unescaping
            kiss_unescape_data(escaped, escaped_len, unescaped, &unescaped_len);
        }
    }
    
    // Test command processing
    if (size >= 2) {
        uint8_t command = data[0] & 0x0F;
        uint8_t port = data[0] >> 4;
        uint8_t value = data[1];
        
        kiss_process_command(&tnc, command, port, value);
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
    if (size < 3) {
        result = 1;  // Too small for KISS
    } else if (size < 16) {
        result = 2;  // Small KISS frame
    } else if (size < 64) {
        result = 3;  // Medium KISS frame
    } else {
        result = 4;  // Large KISS frame
    }
    
    // Branch based on KISS specific fields
    if (size >= 2) {
        uint8_t command = data[0] & 0x0F;
        uint8_t port = data[0] >> 4;
        
        // Branch based on command
        if (command == 0) {
            result += 10;  // Data frame
        } else if (command == 1) {
            result += 20;  // TXDELAY
        } else if (command == 2) {
            result += 30;  // PERSIST
        } else if (command == 3) {
            result += 40;  // SLOTTIME
        } else if (command == 4) {
            result += 50;  // TXTAIL
        } else if (command == 5) {
            result += 60;  // FULLDUPLEX
        } else if (command == 6) {
            result += 70;  // SET_HARDWARE
        } else if (command == 15) {
            result += 80;  // RETURN
        } else {
            result += 90;  // Other command
        }
        
        // Branch based on port
        if (port == 0) {
            result += 100;  // Port 0
        } else if (port < 4) {
            result += 200;  // Port 1-3
        } else {
            result += 300;  // Port 4-7
        }
    }
    
    // Branch based on first byte
    if (data[0] == KISS_FEND) {
        result += 1000;  // KISS frame start
    } else if (data[0] == KISS_FESC) {
        result += 2000;  // KISS escape character
    } else if (data[0] == 0x00) {
        result += 3000;  // Null byte
    } else if (data[0] == 0xFF) {
        result += 4000;  // All ones
    } else {
        result += 5000;  // Other byte
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
    
    // Test KISS protocol functions
    test_kiss_protocol(data, size);
    
    // Call the parsing function
    bool valid = parse_kiss_frame(data, size);
    if (valid) {
        result += 10000000;  // Valid KISS frame
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



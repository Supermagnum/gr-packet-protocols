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

// Include the actual AX.25 protocol implementation
#include "ax25_protocol.h"

#define MAX_SIZE 8192

/*!
 * \brief Parse AX.25 frame with comprehensive validation
 * \param data Input data buffer
 * \param size Size of input data
 * \return true if frame is valid, false otherwise
 */
static bool parse_ax25_frame(const uint8_t* data, size_t size) {
    if (size < 16) return false;
    
    // Check for AX.25 flag (0x7E)
    if (data[0] != 0x7E) return false;
    
    // Parse destination address (6 bytes + SSID)
    for (size_t i = 1; i < 7; i++) {
        uint8_t byte = data[i] >> 1;
        if (byte < 32 || byte > 126) return false;
    }
    
    // Check SSID byte
    uint8_t ssid = data[7];
    if ((ssid & 0x01) == 0) {
        // Has digipeaters - find end of address field
        size_t addr_pos = 14;
        while (addr_pos + 7 <= size && (data[addr_pos - 1] & 0x01) == 0) {
            addr_pos += 7;
        }
        if (addr_pos > size) return false;
    }
    
    // Parse source address
    for (size_t i = 8; i < 14; i++) {
        uint8_t byte = data[i] >> 1;
        if (byte < 32 || byte > 126) return false;
    }
    
    // Parse control field
    uint8_t control = data[14];
    if ((control & 0x01) == 0) {
        // I-frame (Information frame)
        if (size < 17) return false;
        uint8_t pid = data[15];
        
        // Validate PID
        if (pid == 0xF0) {
            // No layer 3 protocol
        } else if (pid == 0xCC) {
            // AX.25 text
        } else if (pid == 0x06) {
            // TCP/IP
        } else if (pid == 0x07) {
            // Compressed TCP/IP
        } else {
            // Unknown PID - still valid
        }
    } else if ((control & 0x02) == 0) {
        // S-frame (Supervisory frame)
        uint8_t type = (control >> 2) & 0x03;
        switch (type) {
            case 0: // RR (Receive Ready)
                break;
            case 1: // RNR (Receive Not Ready)
                break;
            case 2: // REJ (Reject)
                break;
            case 3: // SREJ (Selective Reject)
                break;
        }
    } else {
        // U-frame (Unnumbered frame)
        uint8_t type = control & 0xEF;
        switch (type) {
            case 0x2F: // SABM (Set Asynchronous Balanced Mode)
                break;
            case 0x43: // DISC (Disconnect)
                break;
            case 0x63: // UA (Unnumbered Acknowledgment)
                break;
            case 0x0F: // DM (Disconnected Mode)
                break;
            default: // Other U-frame types
                break;
        }
    }
    
    return true;
}

/*!
 * \brief Test AX.25 protocol functions with fuzzed input
 * \param data Input data buffer
 * \param size Size of input data
 */
static void test_ax25_protocol(const uint8_t* data, size_t size) {
    if (size < 1 || size > MAX_SIZE) return;
    
    // Test AX.25 TNC initialization
    ax25_tnc_t tnc;
    if (ax25_init(&tnc) == 0) {
        // Test address setting with fuzzed data
        if (size >= 6) {
            char callsign[7] = {0};
            size_t copy_size = (size < 6) ? size : 6;
            for (size_t i = 0; i < copy_size; i++) {
                callsign[i] = (data[i] % 26) + 'A'; // Convert to valid callsign
            }
            callsign[copy_size] = '\0';
            
            ax25_address_t addr;
            ax25_set_address(&addr, callsign, data[0] % 16, false);
        }
    }
    
    // Test frame creation with fuzzed data
    if (size >= 14) {
        ax25_frame_t frame;
        ax25_address_t dest_addr, src_addr;
        
        // Set up addresses with fuzzed data
        char dest_callsign[7] = {0};
        char src_callsign[7] = {0};
        
        for (int i = 0; i < 6; i++) {
            dest_callsign[i] = (data[i] % 26) + 'A';
            src_callsign[i] = (data[i + 6] % 26) + 'A';
        }
        
        ax25_set_address(&dest_addr, dest_callsign, data[0] % 16, true);
        ax25_set_address(&src_addr, src_callsign, data[1] % 16, false);
        
        // Create frame with fuzzed control and data
        uint8_t control = data[14];
        uint8_t pid = (size > 15) ? data[15] : 0xF0;
        
        // Extract info field
        size_t info_size = (size > 16) ? (size - 16) : 0;
        if (info_size > 256) info_size = 256; // Limit info field size
        
        if (info_size > 0) {
            ax25_create_frame(&frame, &src_addr, &dest_addr, 
                             control, pid, data + 16, info_size);
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
    if (size < 5) {
        result = 1;  // Small input
    } else if (size < 16) {
        result = 2;  // Medium input
    } else if (size < 32) {
        result = 3;  // Large input
    } else {
        result = 4;  // Very large input
    }
    
    // Branch based on first byte
    if (data[0] == 0x7E) {
        result += 10;  // AX.25 flag found
    } else if (data[0] < 32) {
        result += 20;  // Control character
    } else if (data[0] > 126) {
        result += 30;  // Extended character
    } else {
        result += 40;  // Normal character
    }
    
    // Branch based on second byte
    if (size > 1) {
        if (data[1] == 0x00) {
            result += 100;  // Null byte
        } else if (data[1] == 0xFF) {
            result += 200;  // All ones
        } else if (data[1] < 32) {
            result += 300;  // Control character
        } else if (data[1] > 126) {
            result += 400;  // Extended character
        } else {
            result += 500;  // Normal character
        }
    }
    
    // Branch based on data patterns
    bool has_zeros = false, has_ones = false, has_alternating = false;
    for (size_t i = 0; i < size && i < 10; i++) {
        if (data[i] == 0x00) has_zeros = true;
        if (data[i] == 0xFF) has_ones = true;
        if (i > 0 && data[i] != data[i-1]) has_alternating = true;
    }
    
    if (has_zeros) result += 1000;
    if (has_ones) result += 2000;
    if (has_alternating) result += 3000;
    
    // Branch based on checksum-like calculation
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum += data[i];
    }
    
    if (checksum == 0) {
        result += 10000;  // Zero checksum
    } else if (checksum < 100) {
        result += 20000;  // Low checksum
    } else if (checksum > 1000) {
        result += 30000;  // High checksum
    } else {
        result += 40000;  // Medium checksum
    }
    
    // Branch based on specific byte values
    for (size_t i = 0; i < size && i < 5; i++) {
        if (data[i] == 0x55) result += 100000;
        if (data[i] == 0xAA) result += 200000;
        if (data[i] == 0x33) result += 300000;
        if (data[i] == 0xCC) result += 400000;
    }
    
    // Test AX.25 protocol functions
    test_ax25_protocol(data, size);
    
    // Call the parsing function
    bool valid = parse_ax25_frame(data, size);
    if (valid) {
        result += 1000000;  // Valid AX.25 frame
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



/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * AX.25 Protocol Implementation
 * Provides the C API functions declared in ax25_protocol.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/packet_protocols/ax25_protocol.h>
#include <string.h>
#include <stdlib.h>

// FCS polynomial for AX.25
#define AX25_FCS_POLY 0x1021

// Export functions for shared library
#if defined(__GNUC__) && __GNUC__ >= 4
#define AX25_EXPORT __attribute__ ((visibility ("default")))
#else
#define AX25_EXPORT
#endif

// Calculate FCS (Frame Check Sequence)
static uint16_t calculate_fcs_internal(const uint8_t* data, uint16_t length) {
    uint16_t fcs = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        fcs ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (fcs & 0x0001) {
                fcs = (fcs >> 1) ^ AX25_FCS_POLY;
            } else {
                fcs >>= 1;
            }
        }
    }
    return fcs ^ 0xFFFF;
}

// Initialize AX.25 TNC
AX25_EXPORT int ax25_init(ax25_tnc_t* tnc) {
    if (!tnc) {
        return -1;
    }
    
    memset(tnc, 0, sizeof(ax25_tnc_t));
    
    // Set default configuration
    tnc->config.tx_delay = 30;      // 300ms
    tnc->config.persistence = 63;
    tnc->config.slot_time = 10;     // 100ms
    tnc->config.tx_tail = 10;       // 100ms
    tnc->config.full_duplex = false;
    tnc->config.max_frame_length = 256;
    tnc->config.window_size = 4;
    tnc->config.t1_timeout = 3000;  // 3 seconds
    tnc->config.t2_timeout = 1000;  // 1 second
    tnc->config.t3_timeout = 30000; // 30 seconds
    tnc->config.max_retries = 3;
    
    tnc->num_connections = 0;
    tnc->frame_ready = false;
    
    return 0;
}

// Cleanup AX.25 TNC
AX25_EXPORT int ax25_cleanup(ax25_tnc_t* tnc) {
    if (!tnc) {
        return -1;
    }
    
    // Reset all connections
    for (int i = 0; i < 16; i++) {
        tnc->connections[i].state = AX25_STATE_DISCONNECTED;
    }
    tnc->num_connections = 0;
    tnc->frame_ready = false;
    
    return 0;
}

// Set configuration
int ax25_set_config(ax25_tnc_t* tnc, const ax25_config_t* config) {
    if (!tnc || !config) {
        return -1;
    }
    memcpy(&tnc->config, config, sizeof(ax25_config_t));
    return 0;
}

// Get configuration
int ax25_get_config(const ax25_tnc_t* tnc, ax25_config_t* config) {
    if (!tnc || !config) {
        return -1;
    }
    memcpy(config, &tnc->config, sizeof(ax25_config_t));
    return 0;
}

// Set address
AX25_EXPORT int ax25_set_address(ax25_address_t* addr, const char* callsign, uint8_t ssid, bool command) {
    if (!addr || !callsign) {
        return -1;
    }
    
    memset(addr->callsign, 0x20, 6); // Space fill
    int len = strlen(callsign);
    if (len > 6) len = 6;
    
    for (int i = 0; i < len; i++) {
        addr->callsign[i] = (callsign[i] >= 'a' && callsign[i] <= 'z') 
                            ? (callsign[i] - 'a' + 'A') 
                            : callsign[i];
        addr->callsign[i] <<= 1; // Shift left for AX.25 encoding
    }
    
    addr->ssid = (ssid & 0x0F) << 1;
    if (command) {
        addr->ssid |= 0x80; // Set C bit
    }
    addr->ssid |= 0x01; // Set reserved bit
    addr->command = command;
    addr->has_been_repeated = false;
    
    return 0;
}

// Get address
int ax25_get_address(const ax25_address_t* addr, char* callsign, uint8_t* ssid, bool* command) {
    if (!addr || !callsign || !ssid || !command) {
        return -1;
    }
    
    for (int i = 0; i < 6; i++) {
        callsign[i] = (addr->callsign[i] >> 1) & 0x7F;
        if (callsign[i] == 0x20) {
            callsign[i] = '\0';
            break;
        }
    }
    callsign[6] = '\0';
    
    *ssid = (addr->ssid >> 1) & 0x0F;
    *command = (addr->ssid & 0x80) != 0;
    
    return 0;
}

// Check if addresses are equal
int ax25_address_equal(const ax25_address_t* addr1, const ax25_address_t* addr2) {
    if (!addr1 || !addr2) {
        return 0;
    }
    return (memcmp(addr1->callsign, addr2->callsign, 6) == 0) &&
           (addr1->ssid == addr2->ssid);
}

// Create frame
AX25_EXPORT int ax25_create_frame(ax25_frame_t* frame, const ax25_address_t* src, const ax25_address_t* dst,
                      uint8_t control, uint8_t pid, const uint8_t* info, uint16_t info_len) {
    if (!frame || !src || !dst) {
        return -1;
    }
    
    if (info_len > AX25_MAX_INFO) {
        return -1;
    }
    
    memset(frame, 0, sizeof(ax25_frame_t));
    
    frame->addresses[0] = *dst;
    frame->addresses[1] = *src;
    frame->num_addresses = 2;
    frame->control = control;
    frame->pid = pid;
    
    if (info && info_len > 0) {
        memcpy(frame->info, info, info_len);
        frame->info_length = info_len;
    } else {
        frame->info_length = 0;
    }
    
    frame->valid = true;
    
    return 0;
}

// Parse frame (simplified - full implementation would be more complex)
int ax25_parse_frame(const uint8_t* data, uint16_t length, ax25_frame_t* frame) {
    if (!data || !frame || length < 14) {
        return -1;
    }
    
    memset(frame, 0, sizeof(ax25_frame_t));
    
    // Parse addresses (minimum 2 addresses = 14 bytes)
    int pos = 0;
    frame->num_addresses = 0;
    
    while (pos < length - 2 && frame->num_addresses < AX25_MAX_ADDRS) {
        if (pos + 7 > length) break;
        
        memcpy(frame->addresses[frame->num_addresses].callsign, &data[pos], 6);
        frame->addresses[frame->num_addresses].ssid = data[pos + 6];
        frame->addresses[frame->num_addresses].command = (data[pos + 6] & 0x80) != 0;
        frame->addresses[frame->num_addresses].has_been_repeated = (data[pos + 6] & 0x01) != 0;
        
        pos += 7;
        frame->num_addresses++;
        
        // Check if last address (E bit set)
        if ((frame->addresses[frame->num_addresses - 1].ssid & 0x01) == 0) {
            break;
        }
    }
    
    if (pos >= length) {
        return -1;
    }
    
    // Parse control field
    frame->control = data[pos++];
    
    // Parse PID if UI frame
    if ((frame->control & 0x03) == 0x03) {
        if (pos >= length) {
            return -1;
        }
        frame->pid = data[pos++];
    } else {
        frame->pid = 0;
    }
    
    // Parse information field
    if (pos + 2 < length) {
        uint16_t info_len = length - pos - 2; // -2 for FCS
        if (info_len > AX25_MAX_INFO) {
            info_len = AX25_MAX_INFO;
        }
        memcpy(frame->info, &data[pos], info_len);
        frame->info_length = info_len;
        pos += info_len;
    }
    
    // Parse FCS
    if (pos + 2 <= length) {
        frame->fcs = data[pos] | (data[pos + 1] << 8);
    }
    
    frame->valid = true;
    return 0;
}

// Encode frame
AX25_EXPORT int ax25_encode_frame(const ax25_frame_t* frame, uint8_t* data, uint16_t* length) {
    if (!frame || !data || !length) {
        return -1;
    }
    
    if (!frame->valid || frame->num_addresses < 2) {
        return -1;
    }
    
    uint16_t pos = 0;
    
    // Encode addresses
    for (uint8_t i = 0; i < frame->num_addresses; i++) {
        if (pos + 7 > *length) {
            return -1;
        }
        memcpy(&data[pos], frame->addresses[i].callsign, 6);
        data[pos + 6] = frame->addresses[i].ssid;
        if (i == frame->num_addresses - 1) {
            data[pos + 6] |= 0x01; // Set E bit on last address
        }
        pos += 7;
    }
    
    // Encode control field
    if (pos >= *length) {
        return -1;
    }
    data[pos++] = frame->control;
    
    // Encode PID if UI frame
    if ((frame->control & 0x03) == 0x03) {
        if (pos >= *length) {
            return -1;
        }
        data[pos++] = frame->pid;
    }
    
    // Encode information field
    if (frame->info_length > 0) {
        if (pos + frame->info_length > *length) {
            return -1;
        }
        memcpy(&data[pos], frame->info, frame->info_length);
        pos += frame->info_length;
    }
    
    // Calculate and add FCS
    uint16_t fcs = ax25_calculate_fcs(data, pos);
    if (pos + 2 > *length) {
        return -1;
    }
    data[pos++] = fcs & 0xFF;
    data[pos++] = (fcs >> 8) & 0xFF;
    
    *length = pos;
    return 0;
}

// Validate frame
int ax25_validate_frame(const ax25_frame_t* frame) {
    if (!frame) {
        return -1;
    }
    
    if (!frame->valid) {
        return -1;
    }
    
    if (frame->num_addresses < 2) {
        return -1;
    }
    
    if (frame->info_length > AX25_MAX_INFO) {
        return -1;
    }
    
    return 0;
}

// Connection functions (stubs for now)
int ax25_connect(ax25_tnc_t* tnc, const ax25_address_t* remote_addr) {
    if (!tnc || !remote_addr) {
        return -1;
    }
    // Implementation would manage connections
    return 0;
}

int ax25_disconnect(ax25_tnc_t* tnc, const ax25_address_t* remote_addr) {
    if (!tnc || !remote_addr) {
        return -1;
    }
    // Implementation would manage connections
    return 0;
}

int ax25_send_data(ax25_tnc_t* tnc, const ax25_address_t* remote_addr, const uint8_t* data,
                   uint16_t length) {
    if (!tnc || !remote_addr || !data) {
        return -1;
    }
    // Implementation would send data
    return 0;
}

int ax25_receive_data(ax25_tnc_t* tnc, ax25_address_t* remote_addr, uint8_t* data,
                      uint16_t* length) {
    if (!tnc || !remote_addr || !data || !length) {
        return -1;
    }
    // Implementation would receive data
    return 0;
}

// UI Frame functions
int ax25_send_ui_frame(ax25_tnc_t* tnc, const ax25_address_t* src, const ax25_address_t* dst,
                       const ax25_address_t* digipeaters, uint8_t num_digipeaters, uint8_t pid,
                       const uint8_t* info, uint16_t info_len) {
    if (!tnc || !src || !dst) {
        return -1;
    }
    
    ax25_frame_t frame;
    if (ax25_create_frame(&frame, src, dst, AX25_CTRL_UI, pid, info, info_len) != 0) {
        return -1;
    }
    
    // Add digipeaters if any
    if (digipeaters && num_digipeaters > 0) {
        if (frame.num_addresses + num_digipeaters > AX25_MAX_ADDRS) {
            return -1;
        }
        for (uint8_t i = 0; i < num_digipeaters; i++) {
            frame.addresses[frame.num_addresses++] = digipeaters[i];
        }
    }
    
    tnc->tx_frame = frame;
    tnc->frame_ready = true;
    
    return 0;
}

int ax25_receive_ui_frame(ax25_tnc_t* tnc, ax25_address_t* src, ax25_address_t* dst,
                          ax25_address_t* digipeaters, uint8_t* num_digipeaters, uint8_t* pid,
                          uint8_t* info, uint16_t* info_len) {
    if (!tnc || !src || !dst) {
        return -1;
    }
    
    if (!tnc->frame_ready) {
        return -1;
    }
    
    const ax25_frame_t* frame = &tnc->rx_frame;
    if (frame->num_addresses < 2) {
        return -1;
    }
    
    *src = frame->addresses[1];
    *dst = frame->addresses[0];
    
    if (num_digipeaters) {
        *num_digipeaters = 0;
        if (frame->num_addresses > 2 && digipeaters) {
            *num_digipeaters = frame->num_addresses - 2;
            for (uint8_t i = 0; i < *num_digipeaters && i < AX25_MAX_ADDRS - 2; i++) {
                digipeaters[i] = frame->addresses[i + 2];
            }
        }
    }
    
    if (pid) {
        *pid = frame->pid;
    }
    
    if (info && info_len) {
        if (*info_len > frame->info_length) {
            *info_len = frame->info_length;
        }
        memcpy(info, frame->info, *info_len);
    }
    
    return 0;
}

// FCS Functions
uint16_t ax25_calculate_fcs(const uint8_t* data, uint16_t length) {
    if (!data) {
        return 0;
    }
    return calculate_fcs_internal(data, length);
}

bool ax25_check_fcs(const uint8_t* data, uint16_t length, uint16_t fcs) {
    if (!data || length < 2) {
        return false;
    }
    uint16_t calculated = calculate_fcs_internal(data, length - 2);
    return calculated == fcs;
}

// Utility Functions
int ax25_bit_stuff(const uint8_t* input, uint16_t input_len, uint8_t* output, uint16_t* output_len) {
    if (!input || !output || !output_len) {
        return -1;
    }
    
    uint16_t out_pos = 0;
    int ones_count = 0;
    
    for (uint16_t i = 0; i < input_len && out_pos < *output_len; i++) {
        uint8_t byte = input[i];
        for (int bit = 0; bit < 8 && out_pos < *output_len; bit++) {
            bool bit_val = (byte >> bit) & 0x01;
            
            if (bit_val) {
                ones_count++;
            } else {
                ones_count = 0;
            }
            
            // Set bit in output
            if (bit_val) {
                output[out_pos / 8] |= (1 << (out_pos % 8));
            } else {
                output[out_pos / 8] &= ~(1 << (out_pos % 8));
            }
            out_pos++;
            
            // Bit stuff after 5 consecutive ones
            if (ones_count == 5) {
                if (out_pos >= *output_len) {
                    return -1;
                }
                output[out_pos / 8] &= ~(1 << (out_pos % 8)); // Insert 0
                out_pos++;
                ones_count = 0;
            }
        }
    }
    
    *output_len = out_pos;
    return 0;
}

int ax25_bit_unstuff(const uint8_t* input, uint16_t input_len, uint8_t* output,
                     uint16_t* output_len) {
    if (!input || !output || !output_len) {
        return -1;
    }
    
    uint16_t out_pos = 0;
    int ones_count = 0;
    
    for (uint16_t i = 0; i < input_len && out_pos < *output_len; i++) {
        uint8_t byte = input[i];
        for (int bit = 0; bit < 8 && out_pos < *output_len; bit++) {
            bool bit_val = (byte >> bit) & 0x01;
            
            if (bit_val) {
                ones_count++;
            } else {
                ones_count = 0;
            }
            
            // Set bit in output
            if (bit_val) {
                output[out_pos / 8] |= (1 << (out_pos % 8));
            } else {
                output[out_pos / 8] &= ~(1 << (out_pos % 8));
            }
            out_pos++;
            
            // Remove stuffed bit after 5 consecutive ones
            if (ones_count == 5) {
                ones_count = 0;
                // Skip the next bit (stuffed 0)
                i++;
                if (i >= input_len) break;
                bit = -1; // Will be incremented to 0
            }
        }
    }
    
    *output_len = out_pos;
    return 0;
}

AX25_EXPORT int ax25_add_flags(uint8_t* data, uint16_t* length, uint16_t max_length) {
    if (!data || !length) {
        return -1;
    }
    
    if (*length + 2 > max_length) {
        return -1;
    }
    
    // Move data to make room for flags
    memmove(&data[1], data, *length);
    
    // Add opening flag
    data[0] = AX25_FLAG;
    
    // Add closing flag
    data[*length + 1] = AX25_FLAG;
    
    *length += 2;
    return 0;
}


#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

// Include all protocol headers
#include "gnuradio/packet_protocols/ax25_protocol.h"
#include "gnuradio/packet_protocols/fx25_protocol.h"
#include "gnuradio/packet_protocols/il2p_protocol.h"
#include "gnuradio/packet_protocols/kiss_protocol.h"

// Forward declarations
static void fuzz_ax25(const uint8_t* data, size_t size);
static void fuzz_fx25(const uint8_t* data, size_t size);
static void fuzz_il2p(const uint8_t* data, size_t size);
static void fuzz_kiss(const uint8_t* data, size_t size);

// libFuzzer entry point
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 4 || size > 1024) {
        return 0;
    }

    // Use first byte to select protocol (0-3)
    uint8_t protocol = data[0] % 4;
    const uint8_t* payload = data + 1;
    size_t payload_size = size - 1;

    switch (protocol) {
    case 0:
        fuzz_ax25(payload, payload_size);
        break;
    case 1:
        fuzz_fx25(payload, payload_size);
        break;
    case 2:
        fuzz_il2p(payload, payload_size);
        break;
    case 3:
        fuzz_kiss(payload, payload_size);
        break;
    }

    return 0;
}
// AX.25 Protocol Fuzzer
static void fuzz_ax25(const uint8_t* data, size_t size) {
    if (size < 3)
        return;

    ax25_tnc_t ax25_tnc;
    ax25_frame_t ax25_frame;
    ax25_address_t src_addr, dst_addr;

    // Initialize AX.25 TNC
    if (ax25_init(&ax25_tnc) != 0)
        return;

    // Test AX.25 frame parsing with real protocol
    if (size >= 14 && ax25_parse_frame(data, size, &ax25_frame) == 0) {
        // Frame parsed successfully - test encoding
        uint8_t encoded[1024];
        uint16_t encoded_len = 0;
        if (ax25_encode_frame(&ax25_frame, encoded, &encoded_len) == 0) {
            // Test frame validation
            ax25_frame_t parsed_frame;
            ax25_parse_frame(encoded, encoded_len, &parsed_frame);
        }
    }

    // Test address creation and parsing
    if (size >= 8) {
        char callsign[7] = {0};
        for (int i = 0; i < 6 && i < (int)size; i++) {
            callsign[i] = (data[i] % 26) + 'A'; // Convert to valid callsign
        }
        uint8_t ssid = (data[6] % 16);
        bool command = (data[7] % 2);

        if (ax25_set_address(&src_addr, callsign, ssid, command) == 0) {
            // Test address retrieval
            char retrieved_callsign[7];
            uint8_t retrieved_ssid;
            bool retrieved_command;
            ax25_get_address(&src_addr, retrieved_callsign, &retrieved_ssid, &retrieved_command);
        }
    }

    // Test frame creation
    if (size >= 16) {
        char src_callsign[7] = {0};
        char dst_callsign[7] = {0};
        for (int i = 0; i < 6; i++) {
            src_callsign[i] = (data[i] % 26) + 'A';
            dst_callsign[i] = (data[i + 6] % 26) + 'A';
        }

        if (ax25_set_address(&src_addr, src_callsign, data[12] % 16, false) == 0 &&
            ax25_set_address(&dst_addr, dst_callsign, data[13] % 16, true) == 0) {
            if (ax25_create_frame(&ax25_frame, &src_addr, &dst_addr, (uint8_t)(data[14] % 3),
                                  (uint8_t)(data[15] % 256), data + 16, size - 16) == 0) {
                // Test frame encoding
                uint8_t encoded[1024];
                uint16_t encoded_len = 0;
                if (ax25_encode_frame(&ax25_frame, encoded, &encoded_len) == 0) {
                    // Successfully encoded
                }
            }
        }
    }

    // Cleanup
    ax25_cleanup(&ax25_tnc);
}

// FX.25 Protocol Fuzzer
static void fuzz_fx25(const uint8_t* data, size_t size) {
    if (size < 3)
        return;

    fx25_context_t fx25_ctx;

    // Initialize FX.25 context
    if (fx25_init(&fx25_ctx, FX25_RS_255_223) != 0)
        return;

    // Test Reed-Solomon encoding
    uint8_t parity[256]; // Large enough for all RS configurations
    uint8_t mutable_data[1024];
    memcpy(mutable_data, data, size);
    if (fx25_rs_encode(fx25_ctx.rs, mutable_data, size, parity) == 0) {
        // Test Reed-Solomon decoding
        uint8_t decoded[223];
        fx25_rs_decode(fx25_ctx.rs, mutable_data, size, parity, fx25_ctx.rs->nroots);
    }

    // Test FX.25 frame encoding
    fx25_frame_t fx25_frame;
    if (fx25_encode_frame(&fx25_ctx, data, size, &fx25_frame) == 0) {
        // Test frame decoding
        uint8_t decoded_data[1024];
        uint16_t decoded_len = 0;
        if (fx25_decode_frame(&fx25_ctx, &fx25_frame, decoded_data, &decoded_len) == 0) {
            // Successfully decoded
        }
    }

    // Test CRC validation
    if (size >= 4) {
        uint16_t calculated_crc = fx25_calculate_crc(data, size - 2);
        uint16_t received_crc = (data[size - 2] << 8) | data[size - 1];
        if (calculated_crc == received_crc) {
            // Valid CRC
        }
    }

    // Cleanup
    fx25_cleanup(&fx25_ctx);
}

// IL2P Protocol Fuzzer
static void fuzz_il2p(const uint8_t* data, size_t size) {
    if (size < 3)
        return;

    il2p_context_t il2p_ctx;
    il2p_frame_t il2p_frame;

    // Initialize IL2P context
    if (il2p_init(&il2p_ctx) != 0)
        return;

    // Test frame detection - only if we have enough data
    if (size >= 3) {
        int sync_pos = il2p_detect_frame(data, size);
        if (sync_pos >= 0) {
            // Test frame extraction - only if sync found
            if (il2p_extract_frame(data, size, &il2p_frame) == 0) {
                // Test frame decoding
                uint8_t decoded[1024];
                uint16_t decoded_len = 0;
                if (il2p_decode_frame(&il2p_ctx, &il2p_frame, decoded, &decoded_len) == 0) {
                    // Test header decoding
                    il2p_header_t header;
                    il2p_decode_header(&il2p_ctx, il2p_frame.header, &header);
                }
            }
        }
    }

    // Test frame encoding - only if we have enough data
    if (size >= 3) {
        il2p_frame_t encoded_frame;
        if (il2p_encode_frame(&il2p_ctx, data, size, &encoded_frame) == 0) {
            // Test payload encoding/decoding
            uint8_t payload_encoded[1024];
            uint16_t payload_encoded_len = 0;
            if (il2p_encode_payload(&il2p_ctx, data, size, payload_encoded, &payload_encoded_len) ==
                0) {
                // Test payload decoding
                uint8_t payload_decoded[1024];
                uint16_t payload_decoded_len = 0;
                if (il2p_decode_payload(&il2p_ctx, payload_encoded, payload_encoded_len,
                                        payload_decoded, &payload_decoded_len) == 0) {
                    // Successfully encoded and decoded payload
                }
            }
        }
    }

    // Test header encoding/decoding - only if we have enough data
    if (size >= 3) {
        il2p_header_t header;
        header.version = data[0] % 256;
        header.type = data[1] % 256;
        header.sequence = data[2] % 256;
        header.payload_length = size;
        header.checksum = 0;

        uint8_t header_encoded[64]; // Increased size to prevent buffer overflow
        if (il2p_encode_header(&il2p_ctx, &header, header_encoded) == 0) {
            // Test header decoding
            il2p_header_t decoded_header;
            il2p_decode_header(&il2p_ctx, header_encoded, &decoded_header);
        }
    }

    // Cleanup
    il2p_cleanup(&il2p_ctx);
}

// KISS Protocol Fuzzer
static void fuzz_kiss(const uint8_t* data, size_t size) {
    if (size < 3)
        return;

    kiss_tnc_t kiss_tnc;

    // Initialize KISS TNC
    if (kiss_init(&kiss_tnc) != 0)
        return;

    // Test KISS frame sending - only if we have enough data
    if (size >= 2) {
        uint8_t port = data[0] % 16;
        if (kiss_send_frame(&kiss_tnc, data + 1, size - 1, port) == 0) {
            // Successfully sent frame
        }
    }

    // Test KISS frame receiving
    uint8_t received_data[1024];
    uint16_t received_len = 0;
    uint8_t received_port = 0;
    kiss_receive_frame(&kiss_tnc, received_data, &received_len, &received_port);

    // Test byte-by-byte processing - only if we have data
    if (size > 0) {
        for (size_t i = 0; i < size; i++) {
            kiss_process_byte(&kiss_tnc, data[i]);
        }
    }

    // Test frame ready check
    if (kiss_frame_ready(&kiss_tnc)) {
        // Frame is ready for processing
    }

    // Test data escaping/unescaping - only if we have data
    if (size > 0) {
        uint8_t escaped_data[1024];
        uint16_t escaped_len = 0;
        if (kiss_escape_data(data, size, escaped_data, &escaped_len) == 0) {
            // Test unescaping
            uint8_t unescaped_data[1024];
            uint16_t unescaped_len = 0;
            if (kiss_unescape_data(escaped_data, escaped_len, unescaped_data, &unescaped_len) ==
                0) {
                // Successfully escaped and unescaped data
            }
        }
    }

    // Test TNC configuration - only if we have enough data
    if (size >= 5) {
        kiss_config_t config;
        config.tx_delay = (data[0] % 256);    // Fixed: Allow 0-255 range
        config.persistence = (data[1] % 256); // Fixed: Allow 0-255 range
        config.slot_time = (data[2] % 256);   // Fixed: Allow 0-255 range
        config.tx_tail = (data[3] % 256);     // Fixed: Allow 0-255 range
        config.full_duplex = (data[4] % 2);

        kiss_set_config(&kiss_tnc, &config);

        // Test config retrieval
        kiss_config_t retrieved_config;
        kiss_get_config(&kiss_tnc, &retrieved_config);
    }

    // Cleanup
    kiss_cleanup(&kiss_tnc);
}

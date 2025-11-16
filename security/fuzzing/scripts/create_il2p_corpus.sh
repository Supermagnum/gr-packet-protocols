#!/bin/bash
# IL2P Fuzzing Corpus Generator for gr-packet-protocols
# Creates realistic IL2P frames for comprehensive fuzzing

set -e

# Use relative paths from script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
CORPUS_DIR="$PROJECT_ROOT/security/fuzzing/corpus/il2p_corpus"
mkdir -p "$CORPUS_DIR"

echo "Creating IL2P fuzzing corpus for gr-packet-protocols..."

# Helper function to create IL2P frame
create_il2p_frame() {
    local header_type="$1"
    local payload_size="$2"
    local payload="$3"
    local name="$4"
    
    # Create IL2P header (13.5 bytes encoded with LDPC)
    # Header type (2 bits) + payload size (10 bits) + sequence (12 bits) + checksum (16 bits)
    local header_byte1=$((header_type << 6 | (payload_size >> 4) & 0x3F))
    local header_byte2=$(((payload_size & 0x0F) << 4) | (RANDOM % 16))
    local header_byte3=$((RANDOM % 256))
    local header_byte4=$((RANDOM % 256))
    local header_byte5=$((RANDOM % 256))
    
    # Create header (simplified - just the first 5 bytes)
    local header=$(printf "%02x%02x%02x%02x%02x" $header_byte1 $header_byte2 $header_byte3 $header_byte4 $header_byte5)
    
    # Create LDPC parity (simplified - just random data)
    local ldpc_parity=""
    local parity_size=$((14 - 5))  # Remaining header bytes
    for ((i=0; i<parity_size; i++)); do
        ldpc_parity+=$(printf "%02x" $((RANDOM % 256)))
    done
    
    # Create payload
    local payload_hex=""
    for ((i=0; i<${#payload}; i++)); do
        local char="${payload:$i:1}"
        local byte=$(printf "%d" "'$char")
        payload_hex+=$(printf "%02x" $byte)
    done
    
    # Create LDPC parity for payload (simplified)
    local payload_ldpc_parity=""
    local payload_parity_size=$((payload_size / 8))  # Simplified parity calculation
    for ((i=0; i<payload_parity_size; i++)); do
        payload_ldpc_parity+=$(printf "%02x" $((RANDOM % 256)))
    done
    
    # Create IL2P frame: header + payload + ldpc_parity
    local frame="${header}${ldpc_parity}${payload_hex}${payload_ldpc_parity}"
    
    echo "$frame" | xxd -r -p > "$CORPUS_DIR/$name"
}

# 1. Valid IL2P frames with different header types
echo "Creating valid IL2P frames..."

# Type 0 - Basic data
create_il2p_frame 0 50 "This is a basic IL2P data frame" "il2p_type_0_basic"

# Type 1 - Acknowledgment
create_il2p_frame 1 10 "ACK" "il2p_type_1_ack"

# Type 2 - Control
create_il2p_frame 2 20 "Control message" "il2p_type_2_control"

# Type 3 - Extended
create_il2p_frame 3 100 "Extended IL2P frame with more data" "il2p_type_3_extended"

# 2. IL2P frames with different payload sizes
echo "Creating IL2P frames with different payload sizes..."

# Small payload
create_il2p_frame 0 10 "Short" "il2p_small_payload"

# Medium payload
create_il2p_frame 0 50 "This is a medium length payload for IL2P testing" "il2p_medium_payload"

# Large payload
large_payload=""
for i in {1..200}; do
    large_payload+="A"
done
create_il2p_frame 0 200 "$large_payload" "il2p_large_payload"

# Maximum payload
max_payload=""
for i in {1..1023}; do
    max_payload+="B"
done
create_il2p_frame 0 1023 "$max_payload" "il2p_max_payload"

# 3. IL2P frames with different data patterns
echo "Creating IL2P frames with different data patterns..."

# All zeros payload
zero_payload=""
for i in {1..50}; do
    zero_payload+="\x00"
done
create_il2p_frame 0 50 "$zero_payload" "il2p_zeros_payload"

# All ones payload
ones_payload=""
for i in {1..50}; do
    ones_payload+="\xFF"
done
create_il2p_frame 0 50 "$ones_payload" "il2p_ones_payload"

# Alternating pattern
alt_payload=""
for i in {1..50}; do
    if [ $((i % 2)) -eq 0 ]; then
        alt_payload+="\x55"
    else
        alt_payload+="\xAA"
    fi
done
create_il2p_frame 0 50 "$alt_payload" "il2p_alternating_payload"

# 4. Edge cases
echo "Creating edge cases..."

# Empty payload
create_il2p_frame 0 0 "" "il2p_empty_payload"

# Single byte payload
create_il2p_frame 0 1 "X" "il2p_single_byte"

# Minimum valid frame
printf "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" > "$CORPUS_DIR/il2p_minimum_frame"

# All zeros
dd if=/dev/zero of="$CORPUS_DIR/il2p_all_zeros" bs=64 count=1 2>/dev/null

# All ones
dd if=/dev/zero of="$CORPUS_DIR/il2p_all_ones" bs=64 count=1 2>/dev/null
printf "\xFF" | dd of="$CORPUS_DIR/il2p_all_ones" bs=1 count=64 conv=notrunc 2>/dev/null

# Random data
dd if=/dev/urandom of="$CORPUS_DIR/il2p_random_data" bs=128 count=1 2>/dev/null

# 5. Malformed frames
echo "Creating malformed frames..."

# Invalid header type (> 3)
printf "\xC0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" > "$CORPUS_DIR/il2p_invalid_header_type"

# Invalid payload size (> 1023)
printf "\x3F\xFF\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" > "$CORPUS_DIR/il2p_invalid_payload_size"

# Too short frame
printf "\x00\x00\x00\x00\x00\x00\x00" > "$CORPUS_DIR/il2p_too_short"

# 6. Real-world examples
echo "Creating real-world examples..."

# APRS position report
create_il2p_frame 0 50 "!4903.50N/07201.75W-Test" "il2p_aprs_position"

# APRS message
create_il2p_frame 0 30 ":W1ABC :Hello from N0CALL" "il2p_aprs_message"

# Weather data
create_il2p_frame 0 80 "!4903.50N/07201.75W_180/000g000t068r000p000P000h50b10201L000" "il2p_weather_data"

# 7. Stress test cases
echo "Creating stress test cases..."

# Very long payload (near maximum)
very_long_payload=""
for i in {1..1000}; do
    very_long_payload+="C"
done
create_il2p_frame 0 1000 "$very_long_payload" "il2p_very_long_payload"

# Special characters
create_il2p_frame 0 20 "!@#$%^&*()_+-=[]{}|;':\",./<>?" "il2p_special_chars"

# Binary data
binary_payload=""
for i in {0..255}; do
    binary_payload+=$(printf "\\x%02x" $i)
done
create_il2p_frame 0 256 "$binary_payload" "il2p_binary_data"

# 8. Protocol-specific variations
echo "Creating protocol-specific cases..."

# Different sequence numbers
for seq in {0..15}; do
    create_il2p_frame 0 20 "Sequence $seq test" "il2p_sequence_$seq"
done

# Different checksum values
for checksum in {0..15}; do
    create_il2p_frame 0 20 "Checksum $checksum test" "il2p_checksum_$checksum"
done

# Create summary
echo ""
echo "IL2P Corpus created in: $CORPUS_DIR"
echo "Files created: $(ls -1 "$CORPUS_DIR" | wc -l)"
echo ""
echo "Corpus contents:"
ls -la "$CORPUS_DIR"



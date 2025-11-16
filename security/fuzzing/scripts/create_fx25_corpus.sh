#!/bin/bash
# FX.25 Fuzzing Corpus Generator for gr-packet-protocols
# Creates realistic FX.25 frames for comprehensive fuzzing

set -e

# Use relative paths from script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
CORPUS_DIR="$PROJECT_ROOT/security/fuzzing/corpus/fx25_corpus"
mkdir -p "$CORPUS_DIR"

echo "Creating FX.25 fuzzing corpus for gr-packet-protocols..."

# FX.25 correlation tags for different FEC types
declare -A CORRELATION_TAGS=(
    ["01"]="B74DB7DF8A532F3E"  # RS(255,223)
    ["02"]="26FF60A600CC8FDE"  # RS(255,239)
    ["03"]="C7DC0508F3D9B09E"  # RS(255,247)
    ["04"]="8F056EB4369660EE"  # RS(255,251)
    ["05"]="6E260B1AC5835FAE"  # RS(255,253)
    ["06"]="FF94DC634F1CFF4E"  # RS(255,254)
    ["07"]="1EB7B9CDBC09C00E"  # RS(255,255)
    ["08"]="DBF869BD2DBB1776"  # Custom FEC 1
    ["09"]="3ADB0C13DEDC0826"  # Custom FEC 2
    ["0A"]="AB69DB6A543188D6"  # Custom FEC 3
    ["0B"]="4A4ABEC4A724B796"  # Custom FEC 4
)

# RS parity sizes for each tag
declare -A RS_SIZES=(
    ["01"]="16"
    ["02"]="16"
    ["03"]="32"
    ["04"]="32"
    ["05"]="32"
    ["06"]="48"
    ["07"]="48"
    ["08"]="64"
    ["09"]="64"
    ["0A"]="64"
    ["0B"]="64"
)

# Helper function to create FX.25 frame
create_fx25_frame() {
    local tag="$1"
    local ax25_data="$2"
    local name="$3"
    
    # Get correlation tag
    local correlation_tag="${CORRELATION_TAGS[$tag]}"
    if [ -z "$correlation_tag" ]; then
        echo "Unknown tag: $tag"
        return 1
    fi
    
    # Get RS size
    local rs_size="${RS_SIZES[$tag]}"
    
    # Create RS parity (simplified - just random data)
    local rs_parity=""
    for ((i=0; i<rs_size; i++)); do
        rs_parity+=$(printf "%02x" $((RANDOM % 256)))
    done
    
    # Create FX.25 frame: correlation_tag(8) + rs_parity + ax25_data
    local frame="${correlation_tag}${rs_parity}${ax25_data}"
    
    echo "$frame" | xxd -r -p > "$CORPUS_DIR/$name"
}

# Helper function to create AX.25 frame for FX.25
create_ax25_for_fx25() {
    local dest="$1"
    local src="$2"
    local ctrl="$3"
    local pid="$4"
    local info="$5"
    
    # Convert callsigns to AX.25 format (6 bytes, left-shifted)
    local dest_bytes=""
    for ((i=0; i<6; i++)); do
        if [ $i -lt ${#dest} ]; then
            local char="${dest:$i:1}"
            local byte=$(printf "%d" "'$char")
            dest_bytes+=$(printf "%02x" $((byte << 1)))
        else
            dest_bytes+="40"  # Space
        fi
    done
    
    local src_bytes=""
    for ((i=0; i<6; i++)); do
        if [ $i -lt ${#src} ]; then
            local char="${src:$i:1}"
            local byte=$(printf "%d" "'$char")
            src_bytes+=$(printf "%02x" $((byte << 1)))
        else
            src_bytes+="40"  # Space
        fi
    done
    
    # Set address extension bits
    local dest_last=$(printf "%02x" $((0x01)))  # Last address, not repeated
    local src_last=$(printf "%02x" $((0x01)))   # Last address, not repeated
    
    # Create AX.25 frame: dest(7) + src(7) + ctrl(1) + pid(1) + info
    local ax25_frame="${dest_bytes}${dest_last}${src_bytes}${src_last}${ctrl}${pid}${info}"
    
    echo "$ax25_frame"
}

# 1. Valid FX.25 frames with different FEC types
echo "Creating valid FX.25 frames..."

# FX.25 with RS(255,223) - Tag 01
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "FX.25 test with RS(255,223)")
create_fx25_frame "01" "$ax25_data" "fx25_rs_255_223"

# FX.25 with RS(255,239) - Tag 02
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "FX.25 test with RS(255,239)")
create_fx25_frame "02" "$ax25_data" "fx25_rs_255_239"

# FX.25 with RS(255,247) - Tag 03
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "FX.25 test with RS(255,247)")
create_fx25_frame "03" "$ax25_data" "fx25_rs_255_247"

# FX.25 with RS(255,251) - Tag 04
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "FX.25 test with RS(255,251)")
create_fx25_frame "04" "$ax25_data" "fx25_rs_255_251"

# FX.25 with RS(255,253) - Tag 05
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "FX.25 test with RS(255,253)")
create_fx25_frame "05" "$ax25_data" "fx25_rs_255_253"

# FX.25 with RS(255,254) - Tag 06
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "FX.25 test with RS(255,254)")
create_fx25_frame "06" "$ax25_data" "fx25_rs_255_254"

# FX.25 with RS(255,255) - Tag 07
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "FX.25 test with RS(255,255)")
create_fx25_frame "07" "$ax25_data" "fx25_rs_255_255"

# 2. FX.25 with different AX.25 frame types
echo "Creating FX.25 with different AX.25 frame types..."

# I-frame
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "00" "F0" "FX.25 I-frame test")
create_fx25_frame "01" "$ax25_data" "fx25_i_frame"

# S-frame
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "01" "" "")
create_fx25_frame "02" "$ax25_data" "fx25_s_frame"

# U-frame
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "2F" "" "")
create_fx25_frame "03" "$ax25_data" "fx25_u_frame"

# 3. FX.25 with different payload sizes
echo "Creating FX.25 with different payload sizes..."

# Small payload
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "Short")
create_fx25_frame "01" "$ax25_data" "fx25_small_payload"

# Medium payload
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "This is a medium length payload for FX.25 testing")
create_fx25_frame "02" "$ax25_data" "fx25_medium_payload"

# Large payload
large_payload=""
for i in {1..100}; do
    large_payload+="A"
done
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "$large_payload")
create_fx25_frame "03" "$ax25_data" "fx25_large_payload"

# 4. FX.25 with corrupted AX.25 data
echo "Creating FX.25 with corrupted AX.25 data..."

# Light corruption
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "Corrupted data")
# Corrupt a few bytes
corrupted_ax25="${ax25_data:0:10}FF${ax25_data:12}"
create_fx25_frame "01" "$corrupted_ax25" "fx25_light_corruption"

# Heavy corruption
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "Heavily corrupted data")
# Corrupt multiple bytes
corrupted_ax25="${ax25_data:0:5}FFFFFFFF${ax25_data:13}"
create_fx25_frame "02" "$corrupted_ax25" "fx25_heavy_corruption"

# 5. Edge cases
echo "Creating edge cases..."

# Empty AX.25 data
create_fx25_frame "01" "" "fx25_empty_ax25"

# Single byte AX.25 data
create_fx25_frame "01" "7E" "fx25_single_byte"

# All zeros
dd if=/dev/zero of="$CORPUS_DIR/fx25_all_zeros" bs=64 count=1 2>/dev/null

# All ones
dd if=/dev/zero of="$CORPUS_DIR/fx25_all_ones" bs=64 count=1 2>/dev/null
printf "\xFF" | dd of="$CORPUS_DIR/fx25_all_ones" bs=1 count=64 conv=notrunc 2>/dev/null

# Random data
dd if=/dev/urandom of="$CORPUS_DIR/fx25_random_data" bs=128 count=1 2>/dev/null

# 6. Invalid correlation tags
echo "Creating invalid correlation tags..."

# Invalid tag 1
invalid_tag="0000000000000000"
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "Invalid tag test")
rs_parity=""
for ((i=0; i<16; i++)); do
    rs_parity+=$(printf "%02x" $((RANDOM % 256)))
done
frame="${invalid_tag}${rs_parity}${ax25_data}"
echo "$frame" | xxd -r -p > "$CORPUS_DIR/fx25_invalid_tag_1"

# Invalid tag 2
invalid_tag="FFFFFFFFFFFFFFFF"
frame="${invalid_tag}${rs_parity}${ax25_data}"
echo "$frame" | xxd -r -p > "$CORPUS_DIR/fx25_invalid_tag_2"

# 7. Malformed RS parity
echo "Creating malformed RS parity..."

# Too short RS parity
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "Short RS parity test")
correlation_tag="${CORRELATION_TAGS["01"]}"
short_rs_parity=""
for ((i=0; i<8; i++)); do  # Only 8 bytes instead of 16
    short_rs_parity+=$(printf "%02x" $((RANDOM % 256)))
done
frame="${correlation_tag}${short_rs_parity}${ax25_data}"
echo "$frame" | xxd -r -p > "$CORPUS_DIR/fx25_short_rs_parity"

# Too long RS parity
ax25_data=$(create_ax25_for_fx25 "N0CALL" "W1ABC" "03" "F0" "Long RS parity test")
correlation_tag="${CORRELATION_TAGS["01"]}"
long_rs_parity=""
for ((i=0; i<32; i++)); do  # 32 bytes instead of 16
    long_rs_parity+=$(printf "%02x" $((RANDOM % 256)))
done
frame="${correlation_tag}${long_rs_parity}${ax25_data}"
echo "$frame" | xxd -r -p > "$CORPUS_DIR/fx25_long_rs_parity"

# Create summary
echo ""
echo "FX.25 Corpus created in: $CORPUS_DIR"
echo "Files created: $(ls -1 "$CORPUS_DIR" | wc -l)"
echo ""
echo "Corpus contents:"
ls -la "$CORPUS_DIR"



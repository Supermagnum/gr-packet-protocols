#!/bin/bash
# KISS TNC Fuzzing Corpus Generator for gr-packet-protocols
# Creates realistic KISS frames for comprehensive fuzzing

set -e

# Use relative paths from script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
CORPUS_DIR="$PROJECT_ROOT/security/fuzzing/corpus/kiss_corpus"
mkdir -p "$CORPUS_DIR"

echo "Creating KISS TNC fuzzing corpus for gr-packet-protocols..."

# KISS frame constants
KISS_FEND=0xC0
KISS_FESC=0xDB
KISS_TFEND=0xDC
KISS_TFESC=0xDD

# Helper function to create KISS frame
create_kiss_frame() {
    local port="$1"
    local command="$2"
    local data="$3"
    local name="$4"
    
    # Start with FEND
    local frame=$(printf "%02x" $KISS_FEND)
    
    # Add command and port
    local cmd_byte=$(((port << 4) | command))
    frame+=$(printf "%02x" $cmd_byte)
    
    # Add data with escaping
    for ((i=0; i<${#data}; i++)); do
        local char="${data:$i:1}"
        local byte=$(printf "%d" "'$char")
        
        if [ $byte -eq 192 ]; then  # 0xC0 = KISS_FEND
            # Escape FEND
            frame+=$(printf "%02x%02x" 219 220)  # 0xDB 0xDC
        elif [ $byte -eq 219 ]; then  # 0xDB = KISS_FESC
            # Escape FESC
            frame+=$(printf "%02x%02x" 219 221)  # 0xDB 0xDD
        else
            frame+=$(printf "%02x" $byte)
        fi
    done
    
    # End with FEND
    frame+=$(printf "%02x" $KISS_FEND)
    
    echo "$frame" | xxd -r -p > "$CORPUS_DIR/$name"
}

# Helper function to create KISS command frame
create_kiss_command() {
    local port="$1"
    local command="$2"
    local value="$3"
    local name="$4"
    
    # Start with FEND
    local frame=$(printf "%02x" $KISS_FEND)
    
    # Add command and port
    local cmd_byte=$(((port << 4) | command))
    frame+=$(printf "%02x" $cmd_byte)
    
    # Add value
    frame+=$(printf "%02x" $value)
    
    # End with FEND
    frame+=$(printf "%02x" $KISS_FEND)
    
    echo "$frame" | xxd -r -p > "$CORPUS_DIR/$name"
}

# 1. Valid KISS data frames
echo "Creating valid KISS data frames..."

# Basic data frame
create_kiss_frame 0 0 "Hello KISS" "kiss_data_basic"

# Data frame with different ports
for port in {0..7}; do
    create_kiss_frame $port 0 "Data on port $port" "kiss_data_port_$port"
done

# Data frame with AX.25 data
ax25_data="7E4040404040404040404040404003F0Hello AX.25"
create_kiss_frame 0 0 "$ax25_data" "kiss_ax25_data"

# 2. KISS command frames
echo "Creating KISS command frames..."

# TXDELAY command
create_kiss_command 0 1 50 "kiss_txdelay_50"
create_kiss_command 0 1 100 "kiss_txdelay_100"
create_kiss_command 0 1 200 "kiss_txdelay_200"

# PERSIST command
create_kiss_command 0 2 25 "kiss_persist_25"
create_kiss_command 0 2 50 "kiss_persist_50"
create_kiss_command 0 2 75 "kiss_persist_75"

# SLOTTIME command
create_kiss_command 0 3 10 "kiss_slottime_10"
create_kiss_command 0 3 20 "kiss_slottime_20"
create_kiss_command 0 3 30 "kiss_slottime_30"

# TXTAIL command
create_kiss_command 0 4 5 "kiss_txtail_5"
create_kiss_command 0 4 10 "kiss_txtail_10"
create_kiss_command 0 4 15 "kiss_txtail_15"

# FULLDUPLEX command
create_kiss_command 0 5 0 "kiss_halfduplex"
create_kiss_command 0 5 1 "kiss_fullduplex"

# SET_HARDWARE command
create_kiss_command 0 6 0 "kiss_hardware_0"
create_kiss_command 0 6 1 "kiss_hardware_1"
create_kiss_command 0 6 2 "kiss_hardware_2"

# RETURN command
create_kiss_command 0 15 0 "kiss_return"

# 3. KISS frames with escaped characters
echo "Creating KISS frames with escaped characters..."

# Frame with FEND in data
create_kiss_frame 0 0 "Data with FEND\xC0" "kiss_escaped_fend"

# Frame with FESC in data
create_kiss_frame 0 0 "Data with FESC\xDB" "kiss_escaped_fesc"

# Frame with both FEND and FESC
create_kiss_frame 0 0 "Data with both\xC0\xDB" "kiss_escaped_both"

# 4. Edge cases
echo "Creating edge cases..."

# Empty data frame
create_kiss_frame 0 0 "" "kiss_empty_data"

# Single byte data
create_kiss_frame 0 0 "X" "kiss_single_byte"

# Maximum length data (256 bytes)
max_data=""
for i in {1..256}; do
    max_data+="A"
done
create_kiss_frame 0 0 "$max_data" "kiss_max_length"

# 5. Malformed frames
echo "Creating malformed frames..."

# Missing FEND at start
printf "\x00\x48\x65\x6C\x6C\x6F\xC0" > "$CORPUS_DIR/kiss_missing_start_fend"

# Missing FEND at end
printf "\xC0\x00\x48\x65\x6C\x6C\x6F" > "$CORPUS_DIR/kiss_missing_end_fend"

# No FEND at all
printf "\x00\x48\x65\x6C\x6C\x6F" > "$CORPUS_DIR/kiss_no_fend"

# Double FEND
printf "\xC0\xC0\x00\x48\x65\x6C\x6C\x6F\xC0" > "$CORPUS_DIR/kiss_double_fend"

# 6. Invalid commands
echo "Creating invalid command frames..."

# Invalid command (> 15)
printf "\xC0\xF0\x00\xC0" > "$CORPUS_DIR/kiss_invalid_command"

# Invalid port (> 7)
printf "\xC0\x80\x00\xC0" > "$CORPUS_DIR/kiss_invalid_port"

# 7. Stress test cases
echo "Creating stress test cases..."

# Very long frame
very_long_data=""
for i in {1..1000}; do
    very_long_data+="B"
done
create_kiss_frame 0 0 "$very_long_data" "kiss_very_long"

# Frame with many escapes
escape_data=""
for i in {1..100}; do
    escape_data+="\xC0\xDB"
done
create_kiss_frame 0 0 "$escape_data" "kiss_many_escapes"

# 8. Real-world examples
echo "Creating real-world examples..."

# APRS position report
aprs_data="7E4040404040404040404040404003F0!4903.50N/07201.75W-Test"
create_kiss_frame 0 0 "$aprs_data" "kiss_aprs_position"

# APRS message
aprs_msg="7E4040404040404040404040404003F0:W1ABC :Hello from N0CALL"
create_kiss_frame 0 0 "$aprs_msg" "kiss_aprs_message"

# Weather data
weather_data="7E4040404040404040404040404003F0!4903.50N/07201.75W_180/000g000t068r000p000P000h50b10201L000"
create_kiss_frame 0 0 "$weather_data" "kiss_weather_data"

# 9. Protocol-specific variations
echo "Creating protocol-specific cases..."

# Different baud rates (simulated)
for baud in 1200 2400 4800 9600; do
    create_kiss_command 0 1 $((baud / 100)) "kiss_baud_$baud"
done

# Different TNC configurations
for config in {0..15}; do
    create_kiss_command 0 6 $config "kiss_config_$config"
done

# 10. Binary data
echo "Creating binary data cases..."

# All zeros
printf "\xC0\x00" > "$CORPUS_DIR/kiss_all_zeros"
for i in {1..32}; do
    printf "\x00" >> "$CORPUS_DIR/kiss_all_zeros"
done
printf "\xC0" >> "$CORPUS_DIR/kiss_all_zeros"

# All ones
printf "\xC0\x00" > "$CORPUS_DIR/kiss_all_ones"
for i in {1..32}; do
    printf "\xFF" >> "$CORPUS_DIR/kiss_all_ones"
done
printf "\xC0" >> "$CORPUS_DIR/kiss_all_ones"

# Random data
printf "\xC0\x00" > "$CORPUS_DIR/kiss_random_data"
dd if=/dev/urandom of="$CORPUS_DIR/kiss_random_data" bs=1 count=64 seek=2 2>/dev/null
printf "\xC0" >> "$CORPUS_DIR/kiss_random_data"

# Create summary
echo ""
echo "KISS Corpus created in: $CORPUS_DIR"
echo "Files created: $(ls -1 "$CORPUS_DIR" | wc -l)"
echo ""
echo "Corpus contents:"
ls -la "$CORPUS_DIR"



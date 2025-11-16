#!/bin/bash
# Comprehensive Fuzzing Script for gr-packet-protocols
# Runs fuzzing on all protocol implementations

set -e

# Configuration - Use relative paths from script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
FUZZ_DIR="$PROJECT_ROOT/security/fuzzing"
HARNESS_DIR="$FUZZ_DIR/harnesses"
CORPUS_DIR="$FUZZ_DIR/corpus"
REPORTS_DIR="$FUZZ_DIR/reports"
SCRIPTS_DIR="$FUZZ_DIR/scripts"

# Create reports directory
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT_DIR="$REPORTS_DIR/$TIMESTAMP"
mkdir -p "$REPORT_DIR"

echo "Starting comprehensive fuzzing for gr-packet-protocols..."
echo "Report directory: $REPORT_DIR"

# Function to run fuzzing on a specific harness
run_fuzzing() {
    local harness_name="$1"
    local corpus_name="$2"
    local timeout_minutes="$3"
    
    echo ""
    echo "=========================================="
    echo "Fuzzing $harness_name"
    echo "=========================================="
    
    local harness_file="$HARNESS_DIR/${harness_name}_fuzz.cpp"
    local corpus_path="$CORPUS_DIR/${corpus_name}_corpus"
    local output_dir="$REPORT_DIR/${harness_name}_results"
    
    if [ ! -f "$harness_file" ]; then
        echo "Error: Harness file not found: $harness_file"
        return 1
    fi
    
    if [ ! -d "$corpus_path" ]; then
        echo "Error: Corpus directory not found: $corpus_path"
        return 1
    fi
    
    # Compile harness
    echo "Compiling $harness_name harness..."
    local binary_name="${harness_name}_fuzz"
    local binary_path="$REPORT_DIR/$binary_name"
    
    # Check if we have the protocol headers
    local include_paths="-I$PROJECT_ROOT/include/gnuradio/packet_protocols"
    
    g++ -O2 -fsanitize=address,undefined -fno-omit-frame-pointer \
        $include_paths \
        -o "$binary_path" \
        "$harness_file" \
        "$PROJECT_ROOT/lib/ax25/ax25_protocol.c" \
        "$PROJECT_ROOT/lib/fx25/fx25_protocol.c" \
        "$PROJECT_ROOT/lib/il2p/il2p_protocol.c" \
        "$PROJECT_ROOT/lib/ax25/kiss_protocol.c" \
        2>&1 | tee "$REPORT_DIR/${harness_name}_compile.log"
    
    if [ ${PIPESTATUS[0]} -ne 0 ]; then
        echo "Error: Failed to compile $harness_name harness"
        return 1
    fi
    
    echo "Compilation successful for $harness_name"
    
    # Create output directory
    mkdir -p "$output_dir"
    
    # Run fuzzing with timeout
    echo "Running fuzzing for $harness_name (timeout: ${timeout_minutes} minutes)..."
    
    timeout ${timeout_minutes}m afl-fuzz \
        -i "$corpus_path" \
        -o "$output_dir" \
        -t 1000 \
        -m none \
        -x /dev/null \
        "$binary_path" @@ \
        2>&1 | tee "$REPORT_DIR/${harness_name}_fuzz.log" &
    
    local fuzz_pid=$!
    echo "Fuzzing started for $harness_name with PID: $fuzz_pid"
    
    # Wait for fuzzing to complete or timeout
    wait $fuzz_pid
    local exit_code=$?
    
    if [ $exit_code -eq 124 ]; then
        echo "Fuzzing timed out for $harness_name"
    elif [ $exit_code -eq 0 ]; then
        echo "Fuzzing completed successfully for $harness_name"
    else
        echo "Fuzzing failed for $harness_name with exit code: $exit_code"
    fi
    
    # Generate summary
    if [ -d "$output_dir/default" ]; then
        echo "Generating summary for $harness_name..."
        echo "=== $harness_name Fuzzing Summary ===" > "$REPORT_DIR/${harness_name}_summary.txt"
        echo "Start time: $(date)" >> "$REPORT_DIR/${harness_name}_summary.txt"
        echo "End time: $(date)" >> "$REPORT_DIR/${harness_name}_summary.txt"
        echo "" >> "$REPORT_DIR/${harness_name}_summary.txt"
        
        if [ -f "$output_dir/default/fuzzer_stats" ]; then
            echo "Fuzzer Statistics:" >> "$REPORT_DIR/${harness_name}_summary.txt"
            cat "$output_dir/default/fuzzer_stats" >> "$REPORT_DIR/${harness_name}_summary.txt"
            echo "" >> "$REPORT_DIR/${harness_name}_summary.txt"
        fi
        
        if [ -d "$output_dir/default/crashes" ]; then
            local crash_count=$(ls -1 "$output_dir/default/crashes" 2>/dev/null | wc -l)
            echo "Crashes found: $crash_count" >> "$REPORT_DIR/${harness_name}_summary.txt"
        fi
        
        if [ -d "$output_dir/default/hangs" ]; then
            local hang_count=$(ls -1 "$output_dir/default/hangs" 2>/dev/null | wc -l)
            echo "Hangs found: $hang_count" >> "$REPORT_DIR/${harness_name}_summary.txt"
        fi
    fi
}

# Function to create corpus if it doesn't exist
create_corpus_if_needed() {
    local corpus_name="$1"
    local script_name="create_${corpus_name}_corpus.sh"
    local script_path="$SCRIPTS_DIR/$script_name"
    
    if [ ! -d "$CORPUS_DIR/${corpus_name}_corpus" ]; then
        echo "Creating $corpus_name corpus..."
        if [ -f "$script_path" ]; then
            bash "$script_path"
        else
            echo "Warning: Corpus creation script not found: $script_path"
        fi
    else
        echo "$corpus_name corpus already exists"
    fi
}

# Main fuzzing execution
echo "Setting up fuzzing environment..."

# Check if AFL++ is installed
if ! command -v afl-fuzz &> /dev/null; then
    echo "Error: AFL++ not found. Please install AFL++ first."
    echo "On Ubuntu/Debian: sudo apt install afl++"
    exit 1
fi

# Create corpus for all protocols
echo "Creating corpus for all protocols..."
create_corpus_if_needed "ax25"
create_corpus_if_needed "fx25"
create_corpus_if_needed "il2p"
create_corpus_if_needed "kiss"

# Run fuzzing on all protocols
echo "Starting fuzzing on all protocols..."

# AX.25 fuzzing (10 minutes)
run_fuzzing "ax25_frame" "ax25" 10

# FX.25 fuzzing (10 minutes)
run_fuzzing "fx25_decode" "fx25" 10

# IL2P fuzzing (10 minutes)
run_fuzzing "il2p_decode" "il2p" 10

# KISS fuzzing (10 minutes)
run_fuzzing "kiss_tnc" "kiss" 10

# Generate overall summary
echo ""
echo "=========================================="
echo "Generating overall fuzzing summary..."
echo "=========================================="

cat > "$REPORT_DIR/overall_summary.txt" << EOF
gr-packet-protocols Fuzzing Summary
===================================

Start time: $(date)
End time: $(date)

Protocols tested:
- AX.25 Frame Processing
- FX.25 Decode Processing  
- IL2P Decode Processing
- KISS TNC Processing

Results:
EOF

# Add individual summaries
for protocol in ax25_frame fx25_decode il2p_decode kiss_tnc; do
    if [ -f "$REPORT_DIR/${protocol}_summary.txt" ]; then
        echo "" >> "$REPORT_DIR/overall_summary.txt"
        echo "=== $protocol ===" >> "$REPORT_DIR/overall_summary.txt"
        cat "$REPORT_DIR/${protocol}_summary.txt" >> "$REPORT_DIR/overall_summary.txt"
    fi
done

echo ""
echo "Fuzzing completed!"
echo "Results saved in: $REPORT_DIR"
echo ""
echo "Summary files:"
ls -la "$REPORT_DIR"/*.txt
echo ""
echo "Individual results:"
ls -la "$REPORT_DIR"/*_results



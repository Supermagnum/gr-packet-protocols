#!/bin/bash
# libFuzzer-based fuzzing framework for gr-packet-protocols

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
FUZZ_DIR="$PROJECT_ROOT/security/fuzzing"
HARNESS_DIR="$FUZZ_DIR/harnesses"
CORPUS_DIR="$FUZZ_DIR/corpus"
REPORTS_DIR="$FUZZ_DIR/reports"
SCRIPTS_DIR="$FUZZ_DIR/scripts"

# Create directories
mkdir -p "$CORPUS_DIR" "$REPORTS_DIR"

echo "libFuzzer Fuzzing Framework"
echo "=========================="
echo "Project root: $PROJECT_ROOT"
echo "Harness directory: $HARNESS_DIR"
echo "Corpus directory: $CORPUS_DIR"
echo "Reports directory: $REPORTS_DIR"

# Check if clang++ is available
if ! command -v clang++ &> /dev/null; then
    echo "Error: clang++ not found. Please install clang++ first."
    echo "On Ubuntu/Debian: sudo apt install clang"
    exit 1
fi

# Function to run libFuzzer on a harness
run_libfuzzer() {
    local harness_name="$1"
    local harness_file="$HARNESS_DIR/${harness_name}_libfuzzer.cpp"
    local corpus_dir="$CORPUS_DIR/$harness_name"
    local report_dir="$REPORTS_DIR/${harness_name}_libfuzzer_$(date +%Y%m%d_%H%M%S)"
    
    echo ""
    echo "Fuzzing $harness_name with libFuzzer..."
    echo "Harness: $harness_file"
    echo "Corpus: $corpus_dir"
    echo "Report: $report_dir"
    
    # Create directories
    mkdir -p "$corpus_dir" "$report_dir"
    
        # Compile harness with actual protocol source files
        echo "Compiling $harness_name harness with protocol sources..."
        clang++ -g -O1 -fsanitize=fuzzer,address,undefined \
            -fno-omit-frame-pointer \
            -I"$PROJECT_ROOT/include" \
            "$harness_file" \
            "$PROJECT_ROOT/lib/ax25/ax25_protocol.c" \
            "$PROJECT_ROOT/lib/ax25/kiss_protocol.c" \
            "$PROJECT_ROOT/lib/fx25/fx25_protocol.c" \
            "$PROJECT_ROOT/lib/il2p/il2p_protocol.c" \
            -o "$report_dir/${harness_name}_fuzzer" \
            2> "$report_dir/${harness_name}_compile.log"
    
    if [ $? -ne 0 ]; then
        echo "[FAILED] Compilation failed for $harness_name"
        cat "$report_dir/${harness_name}_compile.log"
        return 1
    fi
    
    echo "[SUCCESS] $harness_name harness compiled"
    
    # Run libFuzzer continuously (no time limit, ignore crashes)
    echo "Running libFuzzer on $harness_name with optimal settings..."
    
    # Set environment variables to reduce crash noise
    export ASAN_OPTIONS="abort_on_error=0:halt_on_error=0:print_stats=0:print_summary=0:log_path=$report_dir/asan_"
    export UBSAN_OPTIONS="abort_on_error=0:halt_on_error=0:print_stats=0:print_summary=0:log_path=$report_dir/ubsan_"
    
    "$report_dir/${harness_name}_fuzzer" \
        -max_total_time=28800 \
        -timeout=60 \
        -rss_limit_mb=4096 \
        -jobs=4 \
        -workers=4 \
        -fork=1 \
        -ignore_crashes=1 \
        -ignore_timeouts=1 \
        -ignore_ooms=1 \
        -print_final_stats=1 \
        -artifact_prefix="$report_dir/" \
        -dedup_token_length=16 \
        -print_corpus_stats=1 \
        -print_coverage=1 \
        -max_len=1024 \
        -corpus_dir="$corpus_dir" \
        > "$report_dir/${harness_name}_fuzzer.log" 2>&1 &
    
    local fuzzer_pid=$!
    echo "libFuzzer PID: $fuzzer_pid"
    
    # Wait for libFuzzer to complete (it will auto-terminate after 8 hours)
    echo "Waiting for libFuzzer to complete (8 hours max)..."
    wait $fuzzer_pid
    local exit_code=$?
    
    echo "libFuzzer completed with exit code: $exit_code"
    
    # Check results
    local crash_count=$(find "$report_dir" -name "crash-*" -type f | wc -l)
    local hang_count=$(find "$report_dir" -name "timeout-*" -type f | wc -l)
    
    echo "Results for $harness_name:"
    echo "  Crashes: $crash_count"
    echo "  Hangs: $hang_count"
    
    if [ $crash_count -gt 0 ]; then
        echo "  Crash files:"
        find "$report_dir" -name "crash-*" -type f | head -3
    fi
    
    if [ $hang_count -gt 0 ]; then
        echo "  Hang files:"
        find "$report_dir" -name "timeout-*" -type f | head -3
    fi
}

# Run fuzzing on all available harnesses
echo "Starting libFuzzer fuzzing campaign..."

# Check for comprehensive harness
if [ ! -f "$HARNESS_DIR/comprehensive_protocol_libfuzzer.cpp" ]; then
    echo "Comprehensive harness not found in $HARNESS_DIR"
    echo "Available files:"
    ls -la "$HARNESS_DIR"
    exit 1
fi

echo "Found comprehensive protocol harness"
echo "Starting comprehensive fuzzing for all protocols..."
echo "All protocols will be tested together for 8 hours"

# Start comprehensive fuzzer
echo "Starting comprehensive 8-hour fuzzing session..."
run_libfuzzer "comprehensive_protocol" &
fuzzer_pid=$!

echo "Comprehensive fuzzer started (PID: $fuzzer_pid)"
echo "Monitoring comprehensive fuzzer for 8 hours..."

# Wait for comprehensive fuzzer to complete (libFuzzer will auto-terminate after 8 hours)
echo "Waiting for comprehensive fuzzer to complete (8 hours max)..."
wait $fuzzer_pid
exit_code=$?

echo "Comprehensive fuzzer completed with exit code: $exit_code"

echo ""
echo "libFuzzer fuzzing campaign complete!"
echo "Results saved in: $REPORTS_DIR"

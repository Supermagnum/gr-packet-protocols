#!/bin/bash
# Quick libFuzzer test for gr-packet-protocols

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FUZZ_DIR="$PROJECT_ROOT/security/fuzzing"
HARNESS_DIR="$FUZZ_DIR/harnesses"
CORPUS_DIR="$FUZZ_DIR/corpus"
REPORTS_DIR="$FUZZ_DIR/reports"

# Create test directory
TEST_DIR="$REPORTS_DIR/quick_libfuzzer_test_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$TEST_DIR"

echo "Quick libFuzzer Test for gr-packet-protocols"
echo "============================================="
echo "Test directory: $TEST_DIR"

# Check if clang++ is available
if ! command -v clang++ &> /dev/null; then
    echo "Error: clang++ not found. Please install clang++ first."
    echo "On Ubuntu/Debian: sudo apt install clang"
    exit 1
fi

# Check corpus
echo "Checking corpus..."
if [ ! -d "$CORPUS_DIR" ] || [ -z "$(ls -A "$CORPUS_DIR" 2>/dev/null)" ]; then
    echo "Generating corpus..."
    bash "$SCRIPT_DIR/create_ax25_corpus.sh"
fi

# Test ultra-minimal harness first
echo ""
echo "Testing ultra-minimal harness..."

# Create ultra-minimal test harness
cat > "$TEST_DIR/ultra_minimal_test.cpp" << 'EOF'
#include <cstdint>
#include <cstdio>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Ultra-minimal test - just return immediately
    (void)data; // Suppress unused parameter warning
    (void)size; // Suppress unused parameter warning
    return 0;
}
EOF

# Compile ultra-minimal harness
echo "Compiling ultra-minimal harness..."
clang++ -fsanitize=fuzzer,address,undefined \
    -g -O1 -fno-omit-frame-pointer \
    "$TEST_DIR/ultra_minimal_test.cpp" \
    -o "$TEST_DIR/ultra_minimal_test" \
    2> "$TEST_DIR/ultra_minimal_compile.log"

if [ $? -eq 0 ]; then
    echo "[SUCCESS] Ultra-minimal harness compiled"
    
    # Test with simple input
    echo "Testing with simple input..."
    echo "test" | timeout 5s "$TEST_DIR/ultra_minimal_test" \
        -max_total_time=5 \
        -timeout=1 \
        -max_len=1024 \
        > "$TEST_DIR/ultra_minimal_test.log" 2>&1
    
    if [ $? -eq 0 ]; then
        echo "[SUCCESS] Ultra-minimal test passed"
    else
        echo "[FAILED] Ultra-minimal test failed"
    fi
else
    echo "[FAILED] Ultra-minimal harness compilation failed"
    cat "$TEST_DIR/ultra_minimal_compile.log"
fi

# Test AX.25 harness
echo ""
echo "Testing AX.25 harness..."

# Compile AX.25 harness
echo "Compiling AX.25 harness..."
clang++ -fsanitize=fuzzer,address,undefined \
    -g -O1 -fno-omit-frame-pointer \
    -I"$PROJECT_ROOT/include" \
    -I"$PROJECT_ROOT/include/gnuradio/packet_protocols" \
    "$HARNESS_DIR/ax25_frame_fuzz.cpp" \
    "$PROJECT_ROOT/lib/ax25/ax25_protocol.c" \
    "$PROJECT_ROOT/lib/ax25/kiss_protocol.c" \
    -o "$TEST_DIR/ax25_frame_test" \
    -lgnuradio-runtime -lgnuradio-blocks \
    2> "$TEST_DIR/ax25_frame_compile.log"

if [ $? -eq 0 ]; then
    echo "[SUCCESS] AX.25 harness compiled"
    
    # Test with corpus
    if [ -d "$CORPUS_DIR/ax25_corpus" ]; then
        echo "Testing with corpus files..."
        timeout 10s "$TEST_DIR/ax25_frame_test" \
            -max_total_time=10 \
            -timeout=1 \
            -max_len=1024 \
            "$CORPUS_DIR/ax25_corpus" \
            > "$TEST_DIR/ax25_frame_test.log" 2>&1
        
        local exit_code=$?
        if [ $exit_code -eq 0 ]; then
            echo "[SUCCESS] AX.25 test passed"
        elif [ $exit_code -eq 124 ]; then
            echo "[TIMEOUT] AX.25 test timed out"
        else
            echo "[FAILED] AX.25 test failed (exit code: $exit_code)"
        fi
    else
        echo "[WARNING] No AX.25 corpus found"
    fi
else
    echo "[FAILED] AX.25 harness compilation failed"
    cat "$TEST_DIR/ax25_frame_compile.log"
fi

# Check for crashes
echo ""
echo "Checking for crashes..."
crash_count=$(find "$TEST_DIR" -name "crash-*" -type f | wc -l)
hang_count=$(find "$TEST_DIR" -name "timeout-*" -type f | wc -l)

if [ $crash_count -gt 0 ]; then
    echo "Found $crash_count crashes"
    find "$TEST_DIR" -name "crash-*" -type f | head -3
fi

if [ $hang_count -gt 0 ]; then
    echo "Found $hang_count hangs"
    find "$TEST_DIR" -name "timeout-*" -type f | head -3
fi

echo ""
echo "Quick libFuzzer test complete!"
echo "Results saved in: $TEST_DIR"



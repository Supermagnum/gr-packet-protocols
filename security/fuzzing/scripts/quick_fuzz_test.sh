#!/bin/bash
# Quick Fuzzing Test for gr-packet-protocols
# Runs a short fuzzing test to verify everything works

set -e

echo "Quick Fuzzing Test for gr-packet-protocols"
echo "=========================================="

# Configuration - Use relative paths from script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
FUZZ_DIR="$PROJECT_ROOT/security/fuzzing"
HARNESS_DIR="$FUZZ_DIR/harnesses"
CORPUS_DIR="$FUZZ_DIR/corpus"
REPORTS_DIR="$FUZZ_DIR/reports"

# Create test reports directory
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_DIR="$REPORTS_DIR/quick_test_$TIMESTAMP"
mkdir -p "$TEST_DIR"

echo "Test directory: $TEST_DIR"

# Function to test a single harness
test_harness() {
    local harness_name="$1"
    local corpus_name="$2"
    
    echo ""
    echo "Testing $harness_name..."
    
    local harness_file="$HARNESS_DIR/${harness_name}_fuzz.cpp"
    local corpus_path="$CORPUS_DIR/${corpus_name}_corpus"
    
    if [ ! -f "$harness_file" ]; then
        echo "Error: Harness file not found: $harness_file"
        return 1
    fi
    
    if [ ! -d "$corpus_path" ]; then
        echo "Error: Corpus directory not found: $corpus_path"
        return 1
    fi
    
    # Compile harness
    echo "Compiling $harness_name..."
    local binary_name="${harness_name}_test"
    local binary_path="$TEST_DIR/$binary_name"
    
    # Compilation with proper include paths
    local include_paths="-I$PROJECT_ROOT/include/gnuradio/packet_protocols"
    
    g++ -O2 -fsanitize=address,undefined -fno-omit-frame-pointer \
        $include_paths \
        -o "$binary_path" \
        "$harness_file" \
        2>&1 | tee "$TEST_DIR/${harness_name}_compile.log"
    
    if [ ${PIPESTATUS[0]} -ne 0 ]; then
        echo "Error: Failed to compile $harness_name"
        return 1
    fi
    
    echo "Compilation successful for $harness_name"
    
    # Test with a few corpus files
    echo "Testing with corpus files..."
    local test_count=0
    local success_count=0
    
    for corpus_file in "$corpus_path"/*; do
        if [ -f "$corpus_file" ]; then
            echo "Testing with: $(basename "$corpus_file")"
            if timeout 5s "$binary_path" "$corpus_file" > "$TEST_DIR/${harness_name}_test_$test_count.log" 2>&1; then
                echo "  [SUCCESS]"
                ((success_count++))
            else
                echo "  [FAILED]"
            fi
            ((test_count++))
            
            # Limit to first 5 files for quick test
            if [ $test_count -ge 5 ]; then
                break
            fi
        fi
    done
    
    echo "Test results for $harness_name: $success_count/$test_count successful"
    
    # Generate summary
    cat > "$TEST_DIR/${harness_name}_summary.txt" << EOF
$harness_name Test Summary
========================
Test time: $(date)
Files tested: $test_count
Successful: $success_count
Failed: $((test_count - success_count))

Compilation log:
$(cat "$TEST_DIR/${harness_name}_compile.log")
EOF
}

# Check if corpus exists, create if needed
echo "Checking corpus..."
if [ ! -d "$CORPUS_DIR/ax25_corpus" ]; then
    echo "Creating AX.25 corpus..."
    bash "$FUZZ_DIR/scripts/create_ax25_corpus.sh"
fi

if [ ! -d "$CORPUS_DIR/fx25_corpus" ]; then
    echo "Creating FX.25 corpus..."
    bash "$FUZZ_DIR/scripts/create_fx25_corpus.sh"
fi

if [ ! -d "$CORPUS_DIR/il2p_corpus" ]; then
    echo "Creating IL2P corpus..."
    bash "$FUZZ_DIR/scripts/create_il2p_corpus.sh"
fi

if [ ! -d "$CORPUS_DIR/kiss_corpus" ]; then
    echo "Creating KISS corpus..."
    bash "$FUZZ_DIR/scripts/create_kiss_corpus.sh"
fi

# Test all harnesses
echo "Testing all harnesses..."

test_harness "ax25_frame" "ax25"
test_harness "fx25_decode" "fx25"
test_harness "il2p_decode" "il2p"
test_harness "kiss_tnc" "kiss"

# Generate overall summary
echo ""
echo "=========================================="
echo "Generating overall test summary..."
echo "=========================================="

cat > "$TEST_DIR/overall_test_summary.txt" << EOF
gr-packet-protocols Quick Fuzzing Test Summary
=============================================

Test time: $(date)
Test directory: $TEST_DIR

Individual test results:
EOF

# Add individual summaries
for protocol in ax25_frame fx25_decode il2p_decode kiss_tnc; do
    if [ -f "$TEST_DIR/${protocol}_summary.txt" ]; then
        echo "" >> "$TEST_DIR/overall_test_summary.txt"
        echo "=== $protocol ===" >> "$TEST_DIR/overall_test_summary.txt"
        cat "$TEST_DIR/${protocol}_summary.txt" >> "$TEST_DIR/overall_test_summary.txt"
    fi
done

echo ""
echo "Quick fuzzing test completed!"
echo "Results saved in: $TEST_DIR"
echo ""
echo "Summary files:"
ls -la "$TEST_DIR"/*.txt
echo ""
echo "Test logs:"
ls -la "$TEST_DIR"/*.log



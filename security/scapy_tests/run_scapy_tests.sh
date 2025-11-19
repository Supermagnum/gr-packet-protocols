#!/bin/bash
# Run Scapy Attack Vector Tests
#
# This script runs all Scapy-based security tests for gr-packet-protocols

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
OUTPUT_DIR="$SCRIPT_DIR/output"

echo "=========================================="
echo "Scapy Attack Vector Testing Suite"
echo "=========================================="
echo ""

# Check for Scapy
if ! python3 -c "import scapy" 2>/dev/null; then
    echo "ERROR: Scapy is not installed"
    echo "Install with: pip install scapy"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Run attack vector tests
echo "Running attack vector tests..."
echo ""

cd "$SCRIPT_DIR"

if python3 test_scapy_attack_vectors.py --output "$OUTPUT_DIR"; then
    echo ""
    echo "[PASS] Attack vector tests completed"
else
    echo ""
    echo "[FAIL] Attack vector tests failed"
    exit 1
fi

echo ""

# Run protocol parser tests (if GNU Radio is available)
if python3 -c "import gnuradio.packet_protocols" 2>/dev/null; then
    echo "Running protocol parser security tests..."
    echo ""
    
    if python3 test_protocol_parsers.py --iterations 10; then
        echo ""
        echo "[PASS] Protocol parser tests completed"
    else
        echo ""
        echo "[FAIL] Protocol parser tests failed"
        exit 1
    fi
else
    echo "Skipping protocol parser tests (GNU Radio not available)"
fi

echo ""
echo "=========================================="
echo "All tests completed"
echo "=========================================="
echo "Test output saved to: $OUTPUT_DIR"


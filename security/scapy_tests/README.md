# Scapy Attack Vector Testing Suite

This directory contains Scapy-based security tests for gr-packet-protocols that validate all attack vectors documented in `SCAPY_ATTACK_VECTORS.md`.

## Overview

The test suite includes:

1. **Attack Vector Tests** (`test_scapy_attack_vectors.py`): Tests all documented attack vectors by generating malicious packets
2. **Protocol Parser Tests** (`test_protocol_parsers.py`): Tests actual protocol parsers with Scapy-generated malicious inputs
3. **Test Runner** (`run_scapy_tests.sh`): Automated test execution script

## Prerequisites

```bash
# Install Scapy
pip install scapy

# For protocol parser tests, GNU Radio must be installed
# (Already required for gr-packet-protocols)
```

## Usage

### Run All Tests

```bash
./run_scapy_tests.sh
```

### Run Specific Tests

```bash
# Run only attack vector generation tests
python3 test_scapy_attack_vectors.py

# Run protocol parser security tests
python3 test_protocol_parsers.py --protocol ax25 --iterations 100

# Test specific protocol
python3 test_scapy_attack_vectors.py --protocol ax25
```

## Test Coverage

### AX.25 Protocol Attacks
- [X] Buffer overflow attacks (oversized frames)
- [X] Malformed frame attacks
- [X] Address spoofing attacks
- [X] Control field manipulation
- [X] Format string attacks
- [X] Integer overflow attempts

### KISS Protocol Attacks
- [X] Buffer overflow attacks
- [X] Escape sequence exploitation
- [X] Command injection attacks
- [X] Malformed frame attacks

### FX.25 Protocol Attacks
- [X] FEC exploitation attacks
- [X] Correlation tag manipulation

### IL2P Protocol Attacks
- [X] Header manipulation attacks
- [X] LDPC exploitation attacks

### Generic Attacks
- [X] Integer overflow attempts
- [X] Format string attacks
- [X] Rate limiting / DoS tests
- [X] Fuzzing with Scapy

### Network Layer Attacks
- [X] Packet injection
- [X] ARP spoofing (test generation)

## Test Output

Test results are saved to `output/` directory:
- Generated malicious packets
- Test execution logs
- Failure reports

## Integration with Fuzzing Framework

These Scapy tests complement the existing fuzzing framework:

- **Fuzzing Framework**: Tests C++ implementations with AFL++/libFuzzer
- **Scapy Tests**: Tests protocol-level attacks and validates security assumptions

Both should be run as part of comprehensive security testing.

## Expected Behavior

All tests should:
1. **Generate malicious packets** without crashing
2. **Validate that parsers handle** malformed input gracefully
3. **Detect buffer overflows** through bounds checking
4. **Identify rate limiting** issues
5. **Report security findings** clearly

## Continuous Integration

These tests can be integrated into CI/CD pipelines:

```yaml
# Example GitHub Actions
- name: Run Scapy Security Tests
  run: |
    pip install scapy
    cd security/scapy_tests
    ./run_scapy_tests.sh
```

## Security Notes

**IMPORTANT**: These tests generate malicious packets for security testing purposes only. Do not use these scripts to attack systems without authorization.

The tests are designed to:
- Validate security of gr-packet-protocols implementations
- Identify vulnerabilities before deployment
- Ensure robust error handling
- Test rate limiting and resource management

## Related Documentation

- [SCAPY_ATTACK_VECTORS.md](../SCAPY_ATTACK_VECTORS.md) - Complete attack vector analysis
- [SECURITY_AUDIT_GUIDE.md](../SECURITY_AUDIT_GUIDE.md) - Security audit procedures
- [FUZZING_FRAMEWORK.md](../FUZZING_FRAMEWORK.md) - Fuzzing framework documentation


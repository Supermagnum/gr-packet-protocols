# Fuzzing Framework Documentation

This document describes the comprehensive fuzzing framework for gr-packet-protocols, designed to identify security vulnerabilities and robustness issues in the protocol implementations.

## Overview

The fuzzing framework uses AFL++ (American Fuzzy Lop) to perform automated security testing of all protocol implementations. It includes:

- **Protocol-specific harnesses** for AX.25, FX.25, IL2P, and KISS TNC
- **Corpus generation** for realistic test data
- **Automated execution** with comprehensive reporting
- **Security analysis** with crash detection and hang detection

## Framework Components

### 1. Fuzzing Harnesses
Located in `security/fuzzing/harnesses/`:

- **`ax25_frame_fuzz.cpp`**: Tests AX.25 frame parsing and validation
- **`fx25_decode_fuzz.cpp`**: Tests FX.25 FEC decoding
- **`il2p_decode_fuzz.cpp`**: Tests IL2P frame processing
- **`kiss_tnc_fuzz.cpp`**: Tests KISS TNC protocol handling

### 2. Corpus Generation Scripts
Located in `security/fuzzing/scripts/`:

- **`create_ax25_corpus.sh`**: Generates AX.25 frame test data
- **`create_fx25_corpus.sh`**: Generates FX.25 frame test data
- **`create_il2p_corpus.sh`**: Generates IL2P frame test data
- **`create_kiss_corpus.sh`**: Generates KISS TNC test data

### 3. Execution Scripts
Located in `security/fuzzing/scripts/`:

- **`run_fuzzing.sh`**: Main fuzzing execution script
- **`quick_fuzz_test.sh`**: Quick verification test
- **`security-audit.sh`**: Comprehensive security audit

## Installation and Setup

### Prerequisites
```bash
# Install AFL++ and dependencies
sudo apt update
sudo apt install -y \
    afl++ \
    clang \
    llvm \
    gcc \
    g++ \
    cmake \
    build-essential
```

### Setup Fuzzing Environment
```bash
# Navigate to project directory
cd /path/to/gr-packet-protocols

# Make scripts executable
chmod +x security/fuzzing/scripts/*.sh

# Create necessary directories
mkdir -p security/fuzzing/{corpus,reports,harnesses}
```

## Usage

### Quick Test
```bash
# Run quick fuzzing test to verify setup
bash security/fuzzing/scripts/quick_fuzz_test.sh
```

### Full Fuzzing Session
```bash
# Run comprehensive fuzzing on all protocols
bash security/fuzzing/scripts/run_fuzzing.sh
```

### Individual Protocol Fuzzing
```bash
# Fuzz specific protocol
bash security/fuzzing/scripts/run_fuzzing.sh ax25
bash security/fuzzing/scripts/run_fuzzing.sh fx25
bash security/fuzzing/scripts/run_fuzzing.sh il2p
bash security/fuzzing/scripts/run_fuzzing.sh kiss
```

## Fuzzing Harnesses

### AX.25 Frame Fuzzing
**File**: `security/fuzzing/harnesses/ax25_frame_fuzz.cpp`

**Purpose**: Tests AX.25 frame parsing, validation, and protocol functions

**Key Features**:
- Frame structure validation
- Address parsing and validation
- Control field processing
- FCS verification
- Protocol function testing

**Test Cases**:
- Valid AX.25 frames
- Malformed frames
- Edge cases (empty frames, oversized frames)
- Invalid addresses and control fields

### FX.25 Decode Fuzzing
**File**: `security/fuzzing/harnesses/fx25_decode_fuzz.cpp`

**Purpose**: Tests FX.25 Forward Error Correction decoding

**Key Features**:
- Reed-Solomon decoding
- Correlation tag validation
- FEC parameter processing
- Error correction testing

**Test Cases**:
- Valid FX.25 frames
- Corrupted frames
- Invalid FEC parameters
- Edge cases in Reed-Solomon decoding

### IL2P Decode Fuzzing
**File**: `security/fuzzing/harnesses/il2p_decode_fuzz.cpp`

**Purpose**: Tests IL2P frame processing and decoding

**Key Features**:
- Header parsing and validation
- Payload processing
- Scrambling/descrambling
- Reed-Solomon FEC

**Test Cases**:
- Valid IL2P frames
- Malformed headers
- Invalid payload data
- Edge cases in frame processing

### KISS TNC Fuzzing
**File**: `security/fuzzing/harnesses/kiss_tnc_fuzz.cpp`

**Purpose**: Tests KISS TNC protocol handling

**Key Features**:
- Frame escaping/unescaping
- Command processing
- Port management
- Hardware interface simulation

**Test Cases**:
- Valid KISS frames
- Escaped data handling
- Command processing
- Port configuration

## Corpus Generation

### AX.25 Corpus
**Script**: `create_ax25_corpus.sh`

**Generated Test Data**:
- Valid AX.25 frames (UI, I, S, U frames)
- APRS position reports
- Message frames
- Digipeater paths
- Edge cases and malformed frames

### FX.25 Corpus
**Script**: `create_fx25_corpus.sh`

**Generated Test Data**:
- FEC-protected frames
- Different Reed-Solomon parameters
- Correlation tags
- Error correction scenarios

### IL2P Corpus
**Script**: `create_il2p_corpus.sh`

**Generated Test Data**:
- Data frames
- Control frames
- Various header types
- Scrambled payloads

### KISS TNC Corpus
**Script**: `create_kiss_corpus.sh`

**Generated Test Data**:
- KISS data frames
- Command frames
- Escaped data
- Port configurations

## Execution and Monitoring

### Fuzzing Execution
```bash
# Start fuzzing session
bash security/fuzzing/scripts/run_fuzzing.sh

# Monitor progress
ps aux | grep afl-fuzz

# Check results
ls -la security/fuzzing/reports/
```

### Results Analysis
```bash
# View fuzzing statistics
cat security/fuzzing/reports/*/fuzzer_stats

# Check for crashes
find security/fuzzing/reports/ -name "crashes" -type d

# Analyze hang detection
find security/fuzzing/reports/ -name "hangs" -type d
```

## Security Analysis

### Crash Detection
The framework automatically detects:
- **Segmentation faults**: Memory access violations
- **Abort signals**: Assertion failures
- **Stack overflows**: Buffer overflow conditions
- **Heap corruption**: Memory management issues

### Hang Detection
The framework identifies:
- **Infinite loops**: Non-terminating code paths
- **Deadlocks**: Resource contention issues
- **Timeout conditions**: Unresponsive code sections

### Vulnerability Categories
- **Buffer overflows**: Stack and heap buffer overruns
- **Integer overflows**: Arithmetic overflow conditions
- **Format string vulnerabilities**: Unsafe string formatting
- **Use-after-free**: Memory management errors
- **Double-free**: Multiple memory deallocation

## Reporting

### Fuzzing Reports
Located in `security/fuzzing/reports/`:

- **`fuzzer_stats`**: Fuzzing statistics and progress
- **`crashes/`**: Crash-inducing inputs
- **`hangs/`**: Hang-inducing inputs
- **`queue/`**: Fuzzing queue and corpus
- **`summary.txt`**: Overall fuzzing summary

### Security Audit Reports
Located in `security/audit/reports/`:

- **`cppcheck-report.xml`**: Static analysis results
- **`flawfinder-report.html`**: Security-focused analysis
- **`semgrep-report.json`**: Custom security rules results

## Best Practices

### Fuzzing Strategy
1. **Start with corpus**: Use realistic test data
2. **Run extended sessions**: Allow sufficient time for coverage
3. **Monitor resources**: Ensure adequate system resources
4. **Analyze results**: Thoroughly investigate findings
5. **Fix issues**: Address all identified vulnerabilities

### Security Considerations
1. **Input validation**: Ensure all inputs are validated
2. **Bounds checking**: Verify array and buffer bounds
3. **Memory management**: Use safe memory allocation patterns
4. **Error handling**: Implement robust error handling
5. **Testing coverage**: Achieve comprehensive test coverage

## Troubleshooting

### Common Issues
1. **AFL++ not found**: Install AFL++ and ensure it's in PATH
2. **Compilation errors**: Check dependencies and build environment
3. **Timeout issues**: Adjust timeout parameters in scripts
4. **Resource exhaustion**: Monitor system resources during fuzzing

### Debug Tips
1. **Enable verbose output**: Use debug flags in fuzzing scripts
2. **Check logs**: Review compilation and execution logs
3. **Verify setup**: Ensure all prerequisites are installed
4. **Test individually**: Run individual harnesses to isolate issues

## Integration with CI/CD

### Automated Fuzzing
```yaml
# GitHub Actions example
name: Security Fuzzing
on: [push, pull_request]
jobs:
  fuzzing:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install -y afl++ clang llvm
      - name: Run fuzzing
        run: |
          bash security/fuzzing/scripts/run_fuzzing.sh
      - name: Upload results
        uses: actions/upload-artifact@v2
        with:
          name: fuzzing-results
          path: security/fuzzing/reports/
```

## References

- **AFL++ Documentation**: [AFL++ Manual](https://github.com/AFLplusplus/AFLplusplus)
- **Fuzzing Best Practices**: [OWASP Fuzzing Guide](https://owasp.org/www-community/Fuzzing)
- **Security Testing**: [NIST Security Testing Guide](https://csrc.nist.gov/publications/detail/sp/800-115/final)

# Fuzzing Framework for gr-packet-protocols

This document describes the comprehensive fuzzing framework implemented for the gr-packet-protocols module, adapted from the gr-m17 project.

## Overview

The fuzzing framework provides automated security testing for all protocol implementations in gr-packet-protocols:

- **AX.25 Protocol**: Frame parsing, encoding, and validation
- **FX.25 Protocol**: Forward Error Correction and frame processing
- **IL2P Protocol**: Modern packet radio protocol implementation
- **KISS TNC**: Terminal Node Controller interface

## Framework Components

### 1. Fuzzing Harnesses

Located in `security/fuzzing/harnesses/`:

- `ax25_frame_fuzz.cpp` - AX.25 frame parsing and validation
- `fx25_decode_fuzz.cpp` - FX.25 decode processing and FEC
- `il2p_decode_fuzz.cpp` - IL2P frame processing and LDPC
- `kiss_tnc_fuzz.cpp` - KISS TNC command and data processing

Each harness includes:
- Comprehensive input validation
- Protocol-specific parsing logic
- Edge case handling
- Memory safety checks
- Branching for fuzzer guidance

### 2. Corpus Generation

Located in `security/fuzzing/scripts/`:

- `create_ax25_corpus.sh` - Generates realistic AX.25 frames
- `create_fx25_corpus.sh` - Creates FX.25 frames with FEC
- `create_il2p_corpus.sh` - Builds IL2P frames with LDPC
- `create_kiss_corpus.sh` - Produces KISS TNC frames

Each corpus includes:
- Valid protocol frames
- Malformed edge cases
- Stress test data
- Real-world examples
- Binary data patterns

### 3. Fuzzing Execution

Located in `security/fuzzing/scripts/`:

- `run_fuzzing.sh` - Comprehensive fuzzing execution
- `quick_fuzz_test.sh` - Quick validation test

## Usage

### Prerequisites

```bash
# Install AFL++ fuzzer
sudo apt install afl++

# Install development tools
sudo apt install build-essential g++

# Install AddressSanitizer and UBSan
sudo apt install libasan-dev libubsan-dev
```

### Quick Test

Run a quick validation test:

```bash
cd /home/haaken/github-projects/gr-packet-protocols
./security/fuzzing/scripts/quick_fuzz_test.sh
```

### Full Fuzzing

Run comprehensive fuzzing on all protocols:

```bash
cd /home/haaken/github-projects/gr-packet-protocols
./security/fuzzing/scripts/run_fuzzing.sh
```

### Individual Protocol Fuzzing

```bash
# Create corpus for specific protocol
./security/fuzzing/scripts/create_ax25_corpus.sh

# Run fuzzing on specific harness
afl-fuzz -i security/fuzzing/corpus/ax25_corpus \
         -o security/fuzzing/reports/ax25_results \
         -t 1000 -m none \
         security/fuzzing/harnesses/ax25_frame_fuzz @@
```

## Fuzzing Harnesses Details

### AX.25 Frame Fuzzing

**Purpose**: Tests AX.25 frame parsing, validation, and protocol functions.

**Key Features**:
- Frame structure validation
- Address field parsing
- Control field processing
- Protocol function testing
- Memory safety checks

**Test Cases**:
- Valid AX.25 frames (I, S, U frames)
- Malformed frames
- Edge cases (empty, oversized)
- Real-world APRS data
- Stress test patterns

### FX.25 Decode Fuzzing

**Purpose**: Tests FX.25 forward error correction and frame processing.

**Key Features**:
- Correlation tag detection
- Reed-Solomon decoding
- FEC validation
- Frame structure parsing
- Error correction testing

**Test Cases**:
- Valid FX.25 frames
- Invalid correlation tags
- Malformed RS parity
- Corrupted AX.25 data
- Different FEC types

### IL2P Decode Fuzzing

**Purpose**: Tests IL2P modern packet radio protocol processing.

**Key Features**:
- Header parsing and validation
- LDPC decoding
- Payload processing
- Data scrambling/descrambling
- Frame structure validation

**Test Cases**:
- Valid IL2P frames
- Different header types
- Various payload sizes
- Malformed headers
- Binary data patterns

### KISS TNC Fuzzing

**Purpose**: Tests KISS Terminal Node Controller interface.

**Key Features**:
- Command processing
- Data frame handling
- Character escaping
- Port and command validation
- TNC configuration

**Test Cases**:
- Valid KISS frames
- Command frames
- Escaped characters
- Malformed frames
- Different ports and commands

## Corpus Generation

### AX.25 Corpus

Generates realistic AX.25 frames including:
- APRS position reports
- APRS messages
- I, S, U frame types
- Different callsign formats
- SSID variations
- Digipeater paths
- Edge cases and malformed frames

### FX.25 Corpus

Creates FX.25 frames with:
- Different FEC types (RS(255,223) to RS(255,255))
- Various correlation tags
- RS parity variations
- Corrupted AX.25 data
- Different payload sizes

### IL2P Corpus

Builds IL2P frames with:
- Different header types (0-3)
- Various payload sizes
- LDPC parity data
- Binary data patterns
- Malformed headers

### KISS Corpus

Produces KISS TNC frames with:
- Data frames with different ports
- Command frames (TXDELAY, PERSIST, etc.)
- Escaped characters (FEND, FESC)
- Malformed frames
- Binary data patterns

## Security Considerations

### Memory Safety

All harnesses include:
- AddressSanitizer (ASan) for buffer overflow detection
- UndefinedBehaviorSanitizer (UBSan) for undefined behavior
- Stack protection
- Heap protection

### Input Validation

Comprehensive validation includes:
- Bounds checking
- Type validation
- Format verification
- Edge case handling
- Malformed input processing

### Protocol Security

Security testing covers:
- Buffer overflow vulnerabilities
- Integer overflow issues
- Format string vulnerabilities
- Memory corruption
- Protocol-specific attacks

## Results Analysis

### Crash Detection

Fuzzing results include:
- Crash files in `crashes/` directory
- Hang files in `hangs/` directory
- Fuzzer statistics
- Coverage information

### Performance Metrics

Key metrics tracked:
- Execution speed (execs/sec)
- Coverage growth
- Crash discovery rate
- Hang detection
- Timeout handling

### Reporting

Comprehensive reporting includes:
- Individual protocol summaries
- Overall fuzzing statistics
- Crash analysis
- Performance metrics
- Coverage reports

## Best Practices

### Fuzzing Strategy

1. **Start with corpus**: Use realistic test data
2. **Run long sessions**: Allow time for deep exploration
3. **Monitor progress**: Track coverage and crashes
4. **Analyze results**: Investigate all crashes and hangs
5. **Iterate**: Fix issues and re-run fuzzing

### Corpus Quality

1. **Real-world data**: Include actual protocol captures
2. **Edge cases**: Test boundary conditions
3. **Malformed data**: Include intentionally bad inputs
4. **Stress tests**: Large and complex inputs
5. **Binary patterns**: Various data patterns

### Harness Design

1. **Comprehensive coverage**: Test all code paths
2. **Meaningful branching**: Guide fuzzer exploration
3. **Error handling**: Test failure conditions
4. **Memory safety**: Include sanitizers
5. **Performance**: Optimize for speed

## Integration with CI/CD

### Automated Fuzzing

The framework can be integrated into CI/CD pipelines:

```bash
# Run fuzzing in CI
./security/fuzzing/scripts/quick_fuzz_test.sh

# Check for crashes
if [ -d "security/fuzzing/reports/*/crashes" ]; then
    echo "Fuzzing found crashes!"
    exit 1
fi
```

### Continuous Security

- Regular fuzzing runs
- Crash analysis and fixing
- Coverage monitoring
- Performance tracking
- Security regression testing

## Troubleshooting

### Common Issues

1. **Compilation errors**: Check include paths and dependencies
2. **Corpus not found**: Run corpus generation scripts
3. **AFL++ not found**: Install AFL++ package
4. **Sanitizer errors**: Check compiler flags
5. **Timeout issues**: Adjust timeout values

### Debugging

1. **Check logs**: Review compilation and execution logs
2. **Test manually**: Run harnesses with specific inputs
3. **Verify corpus**: Ensure corpus files are valid
4. **Check permissions**: Ensure scripts are executable
5. **Monitor resources**: Check memory and CPU usage

## Future Enhancements

### Planned Improvements

1. **LibFuzzer integration**: Add LibFuzzer support
2. **Coverage analysis**: Detailed coverage reporting
3. **Mutation strategies**: Advanced input mutation
4. **Parallel fuzzing**: Multi-threaded execution
5. **Cloud fuzzing**: Distributed fuzzing support

### Research Areas

1. **Protocol-specific fuzzing**: Specialized techniques
2. **Machine learning**: ML-guided fuzzing
3. **Symbolic execution**: Combined approaches
4. **Formal verification**: Mathematical validation
5. **Security proofs**: Formal security analysis

## Conclusion

The fuzzing framework provides comprehensive security testing for gr-packet-protocols, ensuring robust and secure protocol implementations. Regular fuzzing helps identify and fix security vulnerabilities before they reach production systems.

For questions or issues, please refer to the project documentation or contact the development team.



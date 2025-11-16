# Security Framework for gr-packet-protocols

This directory contains the comprehensive security framework for the gr-packet-protocols module, adapted from the gr-m17 project.

## Directory Structure

```
security/
├── fuzzing/                    # Fuzzing framework
│   ├── harnesses/              # Fuzzing harnesses
│   │   ├── ax25_frame_fuzz.cpp
│   │   ├── fx25_decode_fuzz.cpp
│   │   ├── il2p_decode_fuzz.cpp
│   │   └── kiss_tnc_fuzz.cpp
│   ├── corpus/                 # Test corpus
│   │   ├── ax25_corpus/
│   │   ├── fx25_corpus/
│   │   ├── il2p_corpus/
│   │   └── kiss_corpus/
│   ├── scripts/                # Fuzzing scripts
│   │   ├── run_fuzzing.sh
│   │   ├── quick_fuzz_test.sh
│   │   ├── create_ax25_corpus.sh
│   │   ├── create_fx25_corpus.sh
│   │   ├── create_il2p_corpus.sh
│   │   └── create_kiss_corpus.sh
│   └── reports/                # Fuzzing results
├── FUZZING_FRAMEWORK.md        # Fuzzing documentation
├── SECURITY_AUDIT_GUIDE.md     # Security audit guide
└── README.md                   # This file
```

## Quick Start

### Prerequisites

```bash
# Install AFL++ fuzzer
sudo apt install afl++

# Install development tools
sudo apt install build-essential g++ libasan-dev libubsan-dev
```

### Quick Test

Run a quick validation test:

```bash
./security/fuzzing/scripts/quick_fuzz_test.sh
```

### Full Fuzzing

Run comprehensive fuzzing on all protocols:

```bash
./security/fuzzing/scripts/run_fuzzing.sh
```

## Fuzzing Harnesses

### AX.25 Frame Fuzzing
- **File**: `harnesses/ax25_frame_fuzz.cpp`
- **Purpose**: Tests AX.25 frame parsing and validation
- **Features**: Frame structure validation, address parsing, control field processing

### FX.25 Decode Fuzzing
- **File**: `harnesses/fx25_decode_fuzz.cpp`
- **Purpose**: Tests FX.25 forward error correction
- **Features**: Correlation tag detection, Reed-Solomon decoding, FEC validation

### IL2P Decode Fuzzing
- **File**: `harnesses/il2p_decode_fuzz.cpp`
- **Purpose**: Tests IL2P modern packet radio protocol
- **Features**: Header parsing, LDPC decoding, payload processing

### KISS TNC Fuzzing
- **File**: `harnesses/kiss_tnc_fuzz.cpp`
- **Purpose**: Tests KISS Terminal Node Controller interface
- **Features**: Command processing, data frame handling, character escaping

## Corpus Generation

Each protocol has a dedicated corpus generation script:

- `create_ax25_corpus.sh` - Generates realistic AX.25 frames
- `create_fx25_corpus.sh` - Creates FX.25 frames with FEC
- `create_il2p_corpus.sh` - Builds IL2P frames with LDPC
- `create_kiss_corpus.sh` - Produces KISS TNC frames

## Security Features

### Memory Safety
- AddressSanitizer (ASan) for buffer overflow detection
- UndefinedBehaviorSanitizer (UBSan) for undefined behavior
- Stack and heap protection

### Input Validation
- Comprehensive bounds checking
- Type validation
- Format verification
- Edge case handling

### Protocol Security
- Buffer overflow vulnerability testing
- Integer overflow detection
- Format string vulnerability testing
- Memory corruption detection

## Results Analysis

### Crash Detection
- Crash files in `crashes/` directory
- Hang files in `hangs/` directory
- Fuzzer statistics and coverage

### Performance Metrics
- Execution speed (execs/sec)
- Coverage growth
- Crash discovery rate
- Timeout handling

## Documentation

- **FUZZING_FRAMEWORK.md** - Comprehensive fuzzing framework documentation
- **SECURITY_AUDIT_GUIDE.md** - Security audit procedures and guidelines

## Best Practices

### Fuzzing Strategy
1. Start with realistic corpus data
2. Run long fuzzing sessions
3. Monitor progress and coverage
4. Analyze all crashes and hangs
5. Iterate and improve

### Corpus Quality
1. Include real-world protocol captures
2. Test boundary conditions
3. Include intentionally malformed data
4. Use stress test patterns
5. Cover binary data patterns

### Harness Design
1. Ensure comprehensive code coverage
2. Use meaningful branching for fuzzer guidance
3. Test all error conditions
4. Include memory safety checks
5. Optimize for execution speed

## Troubleshooting

### Common Issues
- **Compilation errors**: Check include paths and dependencies
- **Corpus not found**: Run corpus generation scripts
- **AFL++ not found**: Install AFL++ package
- **Sanitizer errors**: Check compiler flags
- **Timeout issues**: Adjust timeout values

### Debugging
1. Check compilation and execution logs
2. Test harnesses manually with specific inputs
3. Verify corpus files are valid
4. Ensure scripts are executable
5. Monitor memory and CPU usage

## Integration

### CI/CD Integration
The fuzzing framework can be integrated into CI/CD pipelines:

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

## Contributing

When contributing to the security framework:

1. Follow the existing code style
2. Add comprehensive documentation
3. Include test cases for new features
4. Ensure all scripts are executable
5. Update documentation as needed

## Support

For questions or issues with the security framework:

1. Check the documentation
2. Review troubleshooting guide
3. Test with quick fuzz test
4. Check logs for errors
5. Contact the development team

## License

This security framework is part of gr-packet-protocols and follows the same licensing terms.



# libFuzzer Fuzzing Framework

This directory contains the libFuzzer-based fuzzing framework for gr-packet-protocols.

## Overview

The fuzzing framework has been completely rewritten to use libFuzzer instead of AFL++. This provides better integration with modern C++ toolchains and more reliable fuzzing results.

## Quick Start

### Run All Fuzzers
```bash
./security/fuzzing/scripts/run_libfuzzer.sh
```

### Quick Test
```bash
./security/fuzzing/scripts/quick_libfuzzer_test.sh
```

### Ultra-Simple Test
```bash
./security/fuzzing/scripts/test_ultra_simple.sh
```

## Framework Components

### Scripts
- `run_libfuzzer.sh` - Main fuzzing orchestration script
- `quick_libfuzzer_test.sh` - Quick verification test
- `test_ultra_simple.sh` - Ultra-simple harness test

### Harnesses
- `ax25_frame_libfuzzer.cpp` - AX.25 protocol fuzzing
- `fx25_decode_libfuzzer.cpp` - FX.25 protocol fuzzing  
- `il2p_decode_libfuzzer.cpp` - IL2P protocol fuzzing
- `kiss_tnc_libfuzzer.cpp` - KISS TNC protocol fuzzing
- `ultra_simple_libfuzzer.cpp` - Minimal test harness

### Corpus Generation
- `create_ax25_corpus.sh` - Generate AX.25 test corpus
- `create_fx25_corpus.sh` - Generate FX.25 test corpus
- `create_il2p_corpus.sh` - Generate IL2P test corpus
- `create_kiss_corpus.sh` - Generate KISS TNC test corpus

## Usage

### Prerequisites
```bash
# Install clang++ (required for libFuzzer)
sudo apt install clang

# Verify installation
clang++ --version
```

### Running Fuzzing
```bash
# Start fuzzing campaign
./security/fuzzing/scripts/run_libfuzzer.sh

# Check results
ls -la security/fuzzing/reports/
```

### Monitoring Progress
```bash
# Check running processes
ps aux | grep fuzzer

# Monitor specific fuzzer
tail -f security/fuzzing/reports/[fuzzer_name]/[fuzzer_name]_fuzzer.log
```

## Results

### Report Structure
```
security/fuzzing/reports/
├── [protocol]_libfuzzer_[timestamp]/
│   ├── [protocol]_fuzzer          # Executable
│   ├── [protocol]_compile.log     # Compilation log
│   ├── [protocol]_fuzzer.log      # Fuzzing log
│   ├── crash-*                    # Crash artifacts
│   └── timeout-*                  # Hang artifacts
```

### Understanding Results
- **Crashes**: Memory errors, segmentation faults
- **Hangs**: Timeout conditions, infinite loops
- **Coverage**: Code coverage metrics in logs
- **Corpus**: Input samples that trigger new code paths

## Troubleshooting

### Common Issues
1. **Compilation Errors**: Check clang++ installation
2. **Timeout Issues**: Reduce harness complexity
3. **No Coverage**: Verify harness logic
4. **Permission Errors**: Check file permissions

### Debug Mode
```bash
# Run with verbose output
clang++ -fsanitize=fuzzer,address,undefined -g -O1 \
    security/fuzzing/harnesses/ultra_simple_libfuzzer.cpp \
    -o /tmp/debug_fuzzer

# Run with debugger
gdb /tmp/debug_fuzzer
```

## Performance Tuning

### libFuzzer Options
- `-max_total_time=N` - Maximum runtime in seconds
- `-timeout=N` - Per-input timeout in seconds  
- `-max_len=N` - Maximum input length
- `-corpus_dir=DIR` - Corpus directory
- `-artifact_prefix=PREFIX` - Output prefix for artifacts

### Optimization
- Use `-O1` for faster execution
- Limit input size with `-max_len`
- Set appropriate timeouts
- Monitor memory usage

## Security Considerations

### Sanitizers
- **AddressSanitizer**: Memory error detection
- **UndefinedBehaviorSanitizer**: Undefined behavior detection
- **libFuzzer**: Coverage-guided fuzzing

### Best Practices
- Run fuzzing in isolated environment
- Monitor system resources
- Review all crash artifacts
- Document findings

## Integration

### CI/CD
```yaml
# Example GitHub Actions workflow
- name: Run Fuzzing
  run: |
    ./security/fuzzing/scripts/run_libfuzzer.sh
    # Check for crashes and report
```

### Monitoring
- Set up automated crash reporting
- Monitor fuzzing progress
- Track coverage improvements
- Alert on new findings

## Contributing

### Adding New Harnesses
1. Create `[protocol]_libfuzzer.cpp` in `harnesses/`
2. Implement `LLVMFuzzerTestOneInput` function
3. Add to `run_libfuzzer.sh` script
4. Test with `test_ultra_simple.sh`

### Harness Template
```cpp
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 1) return 0;
    
    // Your fuzzing logic here
    
    return 0;
}
```

## References

- [libFuzzer Documentation](https://llvm.org/docs/LibFuzzer.html)
- [AddressSanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)



# Testing Framework Documentation

This document describes the comprehensive testing framework for gr-packet-protocols, including unit tests, integration tests, and security testing.

## Testing Overview

The gr-packet-protocols module includes multiple layers of testing:

- **Unit Tests**: Individual component testing
- **Integration Tests**: End-to-end protocol testing
- **Performance Tests**: Performance and throughput testing
- **Security Tests**: Fuzzing and vulnerability testing
- **Regression Tests**: Automated regression testing

## Test Structure

```
tests/
├── unit/              # Unit tests for individual components
├── integration/       # Integration tests for complete workflows
├── performance/       # Performance and benchmark tests
├── security/          # Security and fuzzing tests
├── regression/        # Regression test suite
└── fixtures/          # Test data and fixtures
```

## Unit Testing

### C++ Unit Tests
**Framework**: CTest with CppUnit

**Location**: `tests/unit/`

**Test Categories**:
- **Protocol Tests**: Individual protocol function testing
- **Encoder Tests**: Encoder functionality testing
- **Decoder Tests**: Decoder functionality testing
- **Utility Tests**: Helper function testing

**Example Unit Test**:
```cpp
#include <cppunit/extensions/HelperMacros.h>
#include <gnuradio/packet_protocols/ax25_encoder.h>

class test_ax25_encoder : public CppUnit::TestCase
{
public:
    void test_basic_encoding();
    void test_address_handling();
    void test_frame_construction();
    
    CPPUNIT_TEST_SUITE(test_ax25_encoder);
    CPPUNIT_TEST(test_basic_encoding);
    CPPUNIT_TEST(test_address_handling);
    CPPUNIT_TEST(test_frame_construction);
    CPPUNIT_TEST_SUITE_END();
};

void test_ax25_encoder::test_basic_encoding()
{
    auto encoder = gr::packet_protocols::ax25_encoder::make();
    encoder->set_dest_callsign("APRS");
    encoder->set_src_callsign("N0CALL");
    
    std::string data = "Hello World";
    auto encoded = encoder->encode(data);
    
    CPPUNIT_ASSERT(encoded.size() > 0);
    CPPUNIT_ASSERT(encoded[0] == 0x7E);  // Flag
}
```

### Python Unit Tests
**Framework**: unittest

**Location**: `tests/unit/`

**Test Categories**:
- **Python Bindings**: Python API testing
- **Integration**: Python-C++ integration testing
- **Error Handling**: Exception and error testing

**Example Python Test**:
```python
import unittest
from gnuradio import packet_protocols

class TestAx25Encoder(unittest.TestCase):
    def setUp(self):
        self.encoder = packet_protocols.ax25_encoder()
        self.encoder.set_dest_callsign("APRS")
        self.encoder.set_src_callsign("N0CALL")
        self.encoder.set_ssid(0)
    
    def test_basic_encoding(self):
        data = "Hello World"
        encoded = self.encoder.encode(data)
        self.assertIsNotNone(encoded)
        self.assertGreater(len(encoded), 0)
        self.assertEqual(encoded[0], 0x7E)  # Flag
    
    def test_address_validation(self):
        with self.assertRaises(ValueError):
            self.encoder.set_dest_callsign("")  # Empty callsign
        
        with self.assertRaises(ValueError):
            self.encoder.set_dest_callsign("TOOLONG")  # Too long
```

## Integration Testing

### End-to-End Protocol Tests
**Location**: `tests/integration/`

**Test Scenarios**:
- **Complete Workflows**: Full protocol encoding/decoding cycles
- **Multi-Protocol**: Cross-protocol communication testing
- **Real-World Scenarios**: APRS, data transmission, emergency communications

**Example Integration Test**:
```python
import unittest
from gnuradio import gr, packet_protocols

class TestProtocolIntegration(unittest.TestCase):
    def test_ax25_roundtrip(self):
        # Create encoder and decoder
        encoder = packet_protocols.ax25_encoder()
        decoder = packet_protocols.ax25_decoder()
        
        # Configure
        encoder.set_dest_callsign("APRS")
        encoder.set_src_callsign("N0CALL")
        encoder.set_control_field(0x03)  # UI frame
        
        # Test data
        original_data = "Hello World"
        
        # Encode
        encoded = encoder.encode(original_data)
        self.assertIsNotNone(encoded)
        
        # Decode
        decoded = decoder.decode(encoded)
        self.assertIsNotNone(decoded)
        
        # Verify roundtrip
        self.assertEqual(decoded, original_data)
    
    def test_fx25_fec_integration(self):
        # Test FX.25 FEC with AX.25
        ax25_encoder = packet_protocols.ax25_encoder()
        fx25_encoder = packet_protocols.fx25_encoder()
        fx25_decoder = packet_protocols.fx25_decoder()
        ax25_decoder = packet_protocols.ax25_decoder()
        
        # Configure
        ax25_encoder.set_dest_callsign("APRS")
        ax25_encoder.set_src_callsign("N0CALL")
        fx25_encoder.set_fec_type(0)  # Reed-Solomon
        
        # Test data
        data = "Important Message"
        
        # Encode with FEC
        ax25_encoded = ax25_encoder.encode(data)
        fx25_encoded = fx25_encoder.encode(ax25_encoded)
        
        # Decode with FEC
        fx25_decoded = fx25_decoder.decode(fx25_encoded)
        ax25_decoded = ax25_decoder.decode(fx25_decoded)
        
        # Verify
        self.assertEqual(ax25_decoded, data)
```

## Performance Testing

### Benchmark Tests
**Location**: `tests/performance/`

**Performance Metrics**:
- **Throughput**: Frames per second processing
- **Latency**: End-to-end processing time
- **Memory Usage**: Memory consumption during operation
- **CPU Usage**: CPU utilization during processing

**Example Performance Test**:
```python
import time
import psutil
import numpy as np
from gnuradio import packet_protocols

class TestPerformance(unittest.TestCase):
    def test_encoding_throughput(self):
        encoder = packet_protocols.ax25_encoder()
        encoder.set_dest_callsign("APRS")
        encoder.set_src_callsign("N0CALL")
        
        # Test data
        test_data = "Performance test data" * 100
        num_iterations = 1000
        
        # Measure performance
        start_time = time.time()
        start_memory = psutil.Process().memory_info().rss
        
        for _ in range(num_iterations):
            encoded = encoder.encode(test_data)
        
        end_time = time.time()
        end_memory = psutil.Process().memory_info().rss
        
        # Calculate metrics
        total_time = end_time - start_time
        throughput = num_iterations / total_time
        memory_used = end_memory - start_memory
        
        print(f"Throughput: {throughput:.2f} encodings/second")
        print(f"Memory used: {memory_used / 1024 / 1024:.2f} MB")
        
        # Assertions
        self.assertGreater(throughput, 1000)  # At least 1000 encodings/second
        self.assertLess(memory_used, 100 * 1024 * 1024)  # Less than 100MB
```

## Security Testing

### Fuzzing Tests
**Location**: `tests/security/`

**Fuzzing Framework**: AFL++

**Test Categories**:
- **Input Fuzzing**: Malformed input testing
- **Boundary Testing**: Edge case testing
- **Memory Testing**: Buffer overflow testing
- **Protocol Testing**: Protocol-specific fuzzing

**Example Security Test**:
```cpp
#include <cstdint>
#include <cstring>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 1) return 0;
    
    // Test AX.25 decoder with fuzzed input
    ax25_decoder_t decoder;
    ax25_init_decoder(&decoder);
    
    // Process fuzzed data
    for (size_t i = 0; i < size; i++) {
        ax25_process_byte(&decoder, data[i]);
    }
    
    // Cleanup
    ax25_cleanup_decoder(&decoder);
    
    return 0;
}
```

## Regression Testing

### Automated Regression Tests
**Location**: `tests/regression/`

**Test Categories**:
- **API Compatibility**: API change detection
- **Performance Regression**: Performance degradation detection
- **Functionality Regression**: Feature regression detection

**Example Regression Test**:
```python
import unittest
import json
import os
from gnuradio import packet_protocols

class TestRegression(unittest.TestCase):
    def test_api_compatibility(self):
        # Test that all expected methods exist
        encoder = packet_protocols.ax25_encoder()
        
        expected_methods = [
            'set_dest_callsign',
            'set_src_callsign',
            'set_ssid',
            'encode',
            'get_version'
        ]
        
        for method in expected_methods:
            self.assertTrue(hasattr(encoder, method),
                          f"Missing method: {method}")
    
    def test_performance_regression(self):
        # Load baseline performance data
        baseline_file = "tests/fixtures/performance_baseline.json"
        if os.path.exists(baseline_file):
            with open(baseline_file, 'r') as f:
                baseline = json.load(f)
            
            # Run performance test
            encoder = packet_protocols.ax25_encoder()
            start_time = time.time()
            
            for _ in range(1000):
                encoder.encode("Test data")
            
            end_time = time.time()
            current_time = end_time - start_time
            
            # Check for regression
            self.assertLess(current_time, baseline['encoding_time'] * 1.1,
                           "Performance regression detected")
```

## Test Execution

### Running Tests
```bash
# Run all tests
ctest --output-on-failure

# Run specific test categories
ctest -R unit
ctest -R integration
ctest -R performance

# Run Python tests
python -m pytest tests/unit/
python -m pytest tests/integration/
python -m pytest tests/performance/

# Run security tests
bash tests/security/run_fuzzing.sh
```

### Test Configuration
```bash
# Enable verbose output
export CTEST_OUTPUT_ON_FAILURE=1

# Enable parallel execution
export CTEST_PARALLEL_LEVEL=4

# Enable coverage reporting
export ENABLE_COVERAGE=ON
```

## Test Data and Fixtures

### Test Data Generation
**Location**: `tests/fixtures/`

**Data Categories**:
- **Valid Frames**: Correctly formatted protocol frames
- **Invalid Frames**: Malformed frames for error testing
- **Edge Cases**: Boundary condition test data
- **Performance Data**: Large datasets for performance testing

### Fixture Management
```python
import os
import json

class TestFixtures:
    @staticmethod
    def load_ax25_frames():
        """Load AX.25 test frames from fixtures."""
        fixture_file = "tests/fixtures/ax25_frames.json"
        with open(fixture_file, 'r') as f:
            return json.load(f)
    
    @staticmethod
    def generate_test_data(size):
        """Generate test data of specified size."""
        return "Test data " * (size // 10)
```

## Continuous Integration

### GitHub Actions
```yaml
name: Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install -y gnuradio-dev cmake build-essential
      - name: Build
        run: |
          mkdir build && cd build
          cmake ..
          make -j$(nproc)
      - name: Run tests
        run: |
          cd build
          ctest --output-on-failure
      - name: Run Python tests
        run: |
          python -m pytest tests/unit/
```

## Test Reporting

### Coverage Reports
```bash
# Generate coverage report
gcov -r src/
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

### Performance Reports
```bash
# Generate performance report
python tests/performance/generate_report.py
```

## Best Practices

### Test Design
1. **Isolation**: Tests should be independent and isolated
2. **Deterministic**: Tests should produce consistent results
3. **Fast**: Tests should run quickly
4. **Comprehensive**: Tests should cover all functionality
5. **Maintainable**: Tests should be easy to understand and modify

### Test Documentation
1. **Clear Names**: Test names should describe what they test
2. **Comments**: Complex tests should include explanatory comments
3. **Documentation**: Test documentation should explain test purpose
4. **Examples**: Include usage examples in test documentation

## Troubleshooting

### Common Issues
1. **Test Failures**: Check test output for specific error messages
2. **Performance Issues**: Monitor system resources during testing
3. **Memory Leaks**: Use memory debugging tools
4. **Race Conditions**: Use thread-safe testing practices

### Debug Tips
1. **Verbose Output**: Use verbose flags for detailed output
2. **Debug Builds**: Use debug builds for detailed error information
3. **Logging**: Enable logging for test execution
4. **Isolation**: Run individual tests to isolate issues

## References

- **CTest Documentation**: [CMake Testing](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- **CppUnit Documentation**: [CppUnit Manual](http://cppunit.sourceforge.net/doc/cvs/index.html)
- **Python Testing**: [unittest Documentation](https://docs.python.org/3/library/unittest.html)
- **AFL++ Documentation**: [AFL++ Manual](https://github.com/AFLplusplus/AFLplusplus)



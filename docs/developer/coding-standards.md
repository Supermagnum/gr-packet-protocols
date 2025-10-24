# Coding Standards

This document outlines the coding standards and guidelines for the gr-packet-protocols module, following GNU Radio conventions and best practices.

## GNU Radio Coding Guidelines (GREP 0001)

### Include Order
```cpp
// 1. Config header (if needed)
#include "config.h"

// 2. Block implementation header
#include "block_name_impl.h"

// 3. GNU Radio headers
#include <gnuradio/io_signature.h>
#include <gnuradio/block.h>

// 4. Standard C++ headers
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>

// 5. Third-party headers
#include <boost/format.hpp>

// 6. Project headers
#include <gnuradio/packet_protocols/protocol_name.h>
```

### Class Naming Conventions
```cpp
// Block implementation class
class block_name_impl : public block_name
{
public:
    block_name_impl(/* parameters */);
    ~block_name_impl();
    
    // Public methods
    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);
             
private:
    // Private data members (prefixed with d_)
    int d_parameter;
    std::vector<float> d_buffer;
    bool d_initialized;
};
```

### Data Member Prefixes
- **d_**: Private data members
- **s_**: Static data members
- **m_**: Member variables (alternative to d_)

### Indentation and Formatting
- **Indentation**: 4 spaces (no tabs)
- **Line Length**: Maximum 80 characters
- **Braces**: Opening brace on same line
- **Spacing**: One space around operators

```cpp
// Correct formatting
if (condition) {
    do_something();
} else {
    do_something_else();
}

// Function definition
int function_name(int parameter1, 
                  int parameter2,
                  bool flag)
{
    // Implementation
    return result;
}
```

## Documentation Standards

### Doxygen Comments
```cpp
/*!
 * \brief Brief description of the function
 * \param param1 Description of parameter 1
 * \param param2 Description of parameter 2
 * \return Description of return value
 */
int function_name(int param1, int param2);

/*!
 * \brief Brief description of the class
 * \details Detailed description of the class functionality
 */
class class_name
{
    // Implementation
};
```

### File Headers
```cpp
/* -*- c++ -*- */
/*
 * Copyright 2025 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 *
 * gr-packet-protocols is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * gr-packet-protocols is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gr-packet-protocols; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
```

## C++ Standards

### C++17 Features
- **auto**: Use for type deduction
- **constexpr**: For compile-time constants
- **nullptr**: Instead of NULL
- **Range-based loops**: When appropriate
- **Smart pointers**: std::unique_ptr, std::shared_ptr

```cpp
// Good C++17 usage
auto result = calculate_value();
constexpr int MAX_SIZE = 256;
std::unique_ptr<buffer_t> buffer = std::make_unique<buffer_t>();
```

### Memory Management
```cpp
// Use RAII and smart pointers
class resource_manager
{
private:
    std::unique_ptr<resource_t> d_resource;
    
public:
    resource_manager() : d_resource(std::make_unique<resource_t>()) {}
    ~resource_manager() = default;  // Automatic cleanup
};
```

### Error Handling
```cpp
// Use exceptions for error conditions
if (invalid_condition) {
    throw std::invalid_argument("Invalid parameter");
}

// Return error codes for recoverable errors
int process_data(const uint8_t* data, size_t size)
{
    if (!data || size == 0) {
        return -1;  // Invalid input
    }
    
    // Process data
    return 0;  // Success
}
```

## Python Standards

### PEP 8 Compliance
```python
# Function and variable names: snake_case
def process_ax25_frame(frame_data):
    """Process AX.25 frame data."""
    result = []
    for byte in frame_data:
        result.append(process_byte(byte))
    return result

# Class names: PascalCase
class Ax25Encoder:
    """AX.25 encoder implementation."""
    
    def __init__(self, dest_callsign, src_callsign):
        self.dest_callsign = dest_callsign
        self.src_callsign = src_callsign
```

### Type Hints
```python
from typing import List, Optional, Union

def encode_frame(data: bytes, 
                 dest_callsign: str, 
                 src_callsign: str) -> Optional[bytes]:
    """Encode data into AX.25 frame."""
    if not data or not dest_callsign or not src_callsign:
        return None
    # Implementation
    return encoded_frame
```

## Testing Standards

### Unit Tests
```cpp
// C++ unit tests using CTest
#include <cppunit/extensions/HelperMacros.h>

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
```

### Python Tests
```python
import unittest
from gnuradio import packet_protocols

class TestAx25Encoder(unittest.TestCase):
    def test_basic_encoding(self):
        encoder = packet_protocols.ax25_encoder()
        encoder.set_dest_callsign("APRS")
        encoder.set_src_callsign("N0CALL")
        
        # Test encoding
        result = encoder.encode("Hello World")
        self.assertIsNotNone(result)
        self.assertGreater(len(result), 0)
```

## Security Standards

### Input Validation
```cpp
// Always validate input parameters
int process_data(const uint8_t* data, size_t size)
{
    // Validate input
    if (!data) {
        return -1;  // Invalid pointer
    }
    if (size == 0 || size > MAX_FRAME_SIZE) {
        return -1;  // Invalid size
    }
    
    // Process data safely
    return 0;
}
```

### Buffer Management
```cpp
// Use safe buffer operations
void copy_data_safely(const uint8_t* src, uint8_t* dst, size_t size)
{
    if (!src || !dst || size == 0) {
        return;
    }
    
    // Use memcpy for efficiency, but validate bounds
    if (size <= MAX_BUFFER_SIZE) {
        memcpy(dst, src, size);
    }
}
```

## Performance Standards

### Optimization Guidelines
- **Profile First**: Measure before optimizing
- **Avoid Premature Optimization**: Write clear code first
- **Use Appropriate Data Structures**: Choose efficient containers
- **Minimize Memory Allocations**: Reuse buffers when possible

```cpp
// Efficient buffer reuse
class buffer_manager
{
private:
    std::vector<uint8_t> d_buffer;
    
public:
    uint8_t* get_buffer(size_t size)
    {
        if (d_buffer.size() < size) {
            d_buffer.resize(size);
        }
        return d_buffer.data();
    }
};
```

## Code Review Checklist

### Before Submitting
- [ ] Code follows GNU Radio conventions
- [ ] All functions have Doxygen documentation
- [ ] Unit tests cover new functionality
- [ ] No memory leaks or buffer overflows
- [ ] Input validation for all public functions
- [ ] Error handling for all error conditions
- [ ] Performance considerations addressed
- [ ] Security implications reviewed

### Review Criteria
- **Functionality**: Does it work as intended?
- **Performance**: Is it efficient?
- **Security**: Are there vulnerabilities?
- **Maintainability**: Is it easy to understand and modify?
- **Testing**: Is it adequately tested?

## Tools and Automation

### Code Formatting
```bash
# Use clang-format for C++ code
clang-format -i *.cpp *.h

# Use black for Python code
black *.py
```

### Static Analysis
```bash
# Run cppcheck for C++ code
cppcheck --enable=all --inconclusive --std=c++17 *.cpp

# Run semgrep for security issues
semgrep --config=auto --lang=cpp .
```

### Testing
```bash
# Run unit tests
ctest --output-on-failure

# Run Python tests
python -m pytest tests/
```

## References

- **GNU Radio Coding Guidelines**: [GREP 0001](https://github.com/gnuradio/gnuradio/blob/main/docs/grep-0001-coding-guidelines.md)
- **C++ Core Guidelines**: [C++ Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- **Python PEP 8**: [PEP 8 Style Guide](https://www.python.org/dev/peps/pep-0008/)
- **GNU Radio Documentation**: [GNU Radio Manual](https://www.gnuradio.org/doc/doxygen/)

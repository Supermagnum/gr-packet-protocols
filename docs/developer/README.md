# Developer Documentation

This directory contains comprehensive documentation for developers working on the gr-packet-protocols module.

## Documentation Structure

### Core Documentation
- **[Coding Standards](coding-standards.md)**: GNU Radio coding guidelines and best practices
- **[Testing Framework](testing.md)**: Comprehensive testing documentation
- **[Build System](build-system.md)**: CMake build system documentation
- **[Contributing](contributing.md)**: Contribution guidelines and workflow

### Technical Documentation
- **[Architecture](architecture.md)**: System architecture and design
- **[API Design](api-design.md)**: API design principles and patterns
- **[Performance](performance.md)**: Performance optimization guidelines
- **[Security](security.md)**: Security considerations and best practices

## Quick Start for Developers

### Prerequisites
```bash
# Install development dependencies
sudo apt update
sudo apt install -y \
    gnuradio-dev \
    cmake \
    build-essential \
    pkg-config \
    python3-dev \
    python3-numpy \
    python3-scipy \
    doxygen \
    graphviz \
    cppcheck \
    clang-format
```

### Development Setup
```bash
# Clone the repository
git clone https://github.com/Supermagnum/gr-packet-protocols.git
cd gr-packet-protocols

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the module
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Generate documentation
cd ../docs
./generate-docs.sh
```

## Development Workflow

### 1. Code Development
- **Follow Coding Standards**: Adhere to GNU Radio coding guidelines
- **Write Tests**: Include unit tests for new functionality
- **Document Code**: Add Doxygen comments for all public APIs
- **Format Code**: Use clang-format for consistent formatting

### 2. Testing
- **Unit Tests**: Test individual components
- **Integration Tests**: Test complete workflows
- **Performance Tests**: Verify performance requirements
- **Security Tests**: Run fuzzing and security tests

### 3. Code Review
- **Self Review**: Review your own code before submitting
- **Peer Review**: Have others review your code
- **Automated Checks**: Ensure all automated checks pass
- **Documentation**: Verify documentation is complete

### 4. Integration
- **Merge**: Merge approved changes into main branch
- **Release**: Tag releases and create release notes
- **Documentation**: Update documentation for new features

## Coding Standards

### C++ Standards
- **C++17**: Use modern C++ features
- **GNU Radio Conventions**: Follow GNU Radio coding guidelines
- **Documentation**: Use Doxygen comments for all public APIs
- **Formatting**: Use clang-format for consistent code style

### Python Standards
- **PEP 8**: Follow Python style guidelines
- **Type Hints**: Use type hints for function parameters
- **Docstrings**: Include comprehensive docstrings
- **Testing**: Write unit tests for all functions

### Documentation Standards
- **Doxygen**: Use Doxygen for C++ documentation
- **Markdown**: Use Markdown for general documentation
- **Examples**: Include usage examples in documentation
- **Cross-References**: Link related documentation sections

## Testing Framework

### Test Categories
- **Unit Tests**: Individual component testing
- **Integration Tests**: End-to-end workflow testing
- **Performance Tests**: Performance and throughput testing
- **Security Tests**: Fuzzing and vulnerability testing

### Test Execution
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
```

## Build System

### CMake Configuration
- **Modern CMake**: Use modern CMake practices
- **Dependencies**: Proper dependency management
- **Installation**: Correct installation targets
- **Packaging**: Support for packaging systems

### Build Options
```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Enable testing
cmake -DENABLE_TESTING=ON ..

# Enable documentation
cmake -DENABLE_DOCUMENTATION=ON ..
```

## API Design

### Design Principles
- **Consistency**: Consistent API design across protocols
- **Simplicity**: Simple and intuitive interfaces
- **Performance**: Efficient implementation
- **Extensibility**: Support for future enhancements

### API Patterns
- **Factory Pattern**: Use factory functions for object creation
- **RAII**: Resource Acquisition Is Initialization
- **Exception Safety**: Proper exception handling
- **Thread Safety**: Thread-safe implementations where appropriate

## Performance Guidelines

### Optimization Principles
- **Profile First**: Measure before optimizing
- **Avoid Premature Optimization**: Write clear code first
- **Use Appropriate Data Structures**: Choose efficient containers
- **Minimize Memory Allocations**: Reuse buffers when possible

### Performance Testing
- **Benchmarks**: Establish performance baselines
- **Profiling**: Use profiling tools to identify bottlenecks
- **Monitoring**: Monitor performance during development
- **Regression Testing**: Prevent performance regressions

## Security Considerations

### Security Principles
- **Input Validation**: Validate all input parameters
- **Bounds Checking**: Check array and buffer bounds
- **Memory Management**: Use safe memory allocation patterns
- **Error Handling**: Implement robust error handling

### Security Testing
- **Fuzzing**: Use AFL++ for automated security testing
- **Static Analysis**: Use tools like cppcheck and semgrep
- **Code Review**: Security-focused code review
- **Vulnerability Assessment**: Regular security assessments

## Contributing

### Contribution Process
1. **Fork Repository**: Create a fork of the repository
2. **Create Branch**: Create a feature branch for your changes
3. **Make Changes**: Implement your changes following coding standards
4. **Write Tests**: Include tests for new functionality
5. **Update Documentation**: Update relevant documentation
6. **Submit Pull Request**: Submit a pull request for review

### Pull Request Guidelines
- **Clear Description**: Provide a clear description of changes
- **Testing**: Ensure all tests pass
- **Documentation**: Update documentation as needed
- **Review**: Address review feedback promptly

## Documentation Generation

### Doxygen Documentation
```bash
# Generate HTML documentation
./docs/generate-docs.sh

# Generate with LaTeX support
./docs/generate-docs.sh --latex

# Generate PDF documentation
./docs/generate-docs.sh --pdf
```

### Documentation Standards
- **Doxygen Comments**: Use Doxygen for C++ documentation
- **Markdown**: Use Markdown for general documentation
- **Examples**: Include usage examples
- **Cross-References**: Link related sections

## Troubleshooting

### Common Issues
1. **Build Errors**: Check dependencies and CMake configuration
2. **Test Failures**: Review test output for specific errors
3. **Documentation Issues**: Verify Doxygen comment syntax
4. **Performance Issues**: Use profiling tools to identify bottlenecks

### Debug Tips
1. **Verbose Output**: Use verbose flags for detailed output
2. **Debug Builds**: Use debug builds for detailed error information
3. **Logging**: Enable logging for debugging
4. **Isolation**: Test components in isolation

## Resources

### Documentation
- **GNU Radio Manual**: [GNU Radio Documentation](https://www.gnuradio.org/doc/doxygen/)
- **CMake Documentation**: [CMake Manual](https://cmake.org/cmake/help/latest/)
- **Doxygen Manual**: [Doxygen Documentation](https://www.doxygen.nl/manual/)

### Tools
- **clang-format**: Code formatting
- **cppcheck**: Static analysis
- **AFL++**: Fuzzing framework
- **Valgrind**: Memory debugging

### Community
- **GNU Radio Mailing List**: [GNU Radio Community](https://www.gnuradio.org/community/)
- **GitHub Issues**: [Project Issues](https://github.com/Supermagnum/gr-packet-protocols/issues)
- **Discussions**: [Project Discussions](https://github.com/Supermagnum/gr-packet-protocols/discussions)



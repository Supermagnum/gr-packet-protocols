# Commit Guidelines for gr-packet-protocols

This document outlines the commit message guidelines for the gr-packet-protocols module, following GNU Radio conventions as specified in [GREP 0001](https://raw.githubusercontent.com/gnuradio/greps/refs/heads/main/grep-0001-coding-guidelines.md).

## Commit Message Format

### Subject Line
- Prefix with the section of code (e.g., "ax25:", "fx25:", "il2p:", "kiss:", "cmake:", "docs:")
- Use imperative mood (e.g., "Fix buffer overflow" not "Fixed buffer overflow")
- Keep to 50 characters, hard limit of 72 characters
- Examples:
  - `ax25: Fix buffer overflow in encoder`
  - `fx25: Add Reed-Solomon FEC support`
  - `kiss: Implement TNC interface`
  - `cmake: Update build configuration`
  - `docs: Add installation instructions`

### Body
- Separate from subject line with one blank line
- Explain the *what* and *why* of the commit
- Keep lines to 72 characters
- Use proper grammar and spelling
- Reference issues or pull requests when relevant

### Example Commit Messages

```
ax25: Add frame validation to encoder

Implement proper AX.25 frame validation in the encoder to ensure
all outgoing frames meet the AX.25 specification. This prevents
malformed frames from being transmitted and improves compatibility
with standard AX.25 equipment.

- Add FCS calculation validation
- Implement address field validation
- Add frame length checking
- Update unit tests for validation

Fixes #123
```

```
fx25: Implement Reed-Solomon encoding

Add Reed-Solomon forward error correction to FX.25 implementation
based on the gr-m17 protocol code. This provides enhanced error
correction capabilities for noisy radio channels.

- Import RS encoding from gr-m17
- Add multiple FEC code support
- Implement frame detection
- Add comprehensive test cases

Closes #456
```

## Code Style Guidelines

### C++ Formatting
- Use 4 spaces for indentation
- Maximum line length: 100 characters (preferably 80)
- Use clang-format for consistent formatting
- Follow GNU Radio naming conventions

### Class Members
- All class data members begin with `d_` prefix
- All static data members begin with `s_` prefix
- Use descriptive names with underscores

### Documentation
- Use Doxygen doc-blocks for all public interfaces
- Include parameter descriptions
- Document return values and exceptions
- Add usage examples where appropriate

### Include Order
1. Local headers
2. GNU Radio headers
3. 3rd-party library headers
4. Standard headers

Example:
```cpp
#include "ax25_encoder_impl.h"
#include <gnuradio/io_signature.h>
#include <boost/format.hpp>
#include <cstring>
```

## Pre-commit Checklist

Before committing, ensure:

- [ ] Code follows GNU Radio style guidelines
- [ ] All tests pass
- [ ] Documentation is updated
- [ ] Commit message follows format guidelines
- [ ] No trailing whitespace
- [ ] Files end with newline
- [ ] Code is properly formatted with clang-format

## Branch Naming

Use descriptive branch names:
- `feature/ax25-kiss-interface`
- `fix/fx25-fec-bug`
- `docs/update-readme`
- `refactor/il2p-encoder`

## Pull Request Guidelines

- Use descriptive titles
- Include detailed description of changes
- Reference related issues
- Ensure all CI checks pass
- Request review from maintainers





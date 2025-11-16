#include <cstdint>
#include <cstdio>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Ultra-minimal test - just return immediately
    (void)data; // Suppress unused parameter warning
    (void)size; // Suppress unused parameter warning
    return 0;
}

cmake_minimum_required(VERSION 3.16)

project(gr-packet-protocols)

# Find required packages
find_package(PkgConfig)
pkg_check_modules(PC_GNURADIO gnuradio-runtime)

if(NOT PC_GNURADIO_FOUND)
    find_package(Gnuradio "3.10" REQUIRED)
endif()

# Ensure GNURADIO_LIBRARIES is set
if(NOT GNURADIO_LIBRARIES AND PC_GNURADIO_FOUND)
    set(GNURADIO_LIBRARIES ${PC_GNURADIO_LIBRARIES})
endif()

# Use GNU Radio installation paths
# These paths are relative to CMAKE_INSTALL_PREFIX
# For system-wide installation, use CMAKE_INSTALL_PREFIX=/usr
# For local installation, use CMAKE_INSTALL_PREFIX=/usr/local
set(GR_LIBRARY_DIR "${CMAKE_INSTALL_PREFIX}/lib/x86_64-linux-gnu")
set(GR_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
set(GR_PKG_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/gnuradio")

# Detect Python version and set correct installation path
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_path('platlib'))"
        OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    # Extract the path relative to install prefix
    string(REPLACE "${CMAKE_INSTALL_PREFIX}" "" PYTHON_SITE_PACKAGES "${PYTHON_SITE_PACKAGES}")
    string(STRIP "${PYTHON_SITE_PACKAGES}" PYTHON_SITE_PACKAGES)
    if(PYTHON_SITE_PACKAGES)
        set(GR_PYTHON_DIR "${CMAKE_INSTALL_PREFIX}${PYTHON_SITE_PACKAGES}")
    else()
        # Fallback to default path
        set(GR_PYTHON_DIR "${CMAKE_INSTALL_PREFIX}/lib/python3/dist-packages")
    endif()
else()
    # Fallback to default path
    set(GR_PYTHON_DIR "${CMAKE_INSTALL_PREFIX}/lib/python3/dist-packages")
endif()

# Set the version
set(GR_PACKET_PROTOCOLS_VERSION_MAJOR 1)
set(GR_PACKET_PROTOCOLS_VERSION_MINOR 0)
set(GR_PACKET_PROTOCOLS_VERSION_PATCH 0)
set(GR_PACKET_PROTOCOLS_VERSION 
    "${GR_PACKET_PROTOCOLS_VERSION_MAJOR}.${GR_PACKET_PROTOCOLS_VERSION_MINOR}.${GR_PACKET_PROTOCOLS_VERSION_PATCH}")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Enable clang-format if available
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${GR_PACKET_PROTOCOLS_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-format"
    )
endif()

# Define the library
set(GR_PACKET_PROTOCOLS_SOURCES
    lib/ax25/ax25_encoder_impl.cc
    lib/ax25/ax25_decoder_impl.cc
    lib/ax25/kiss_tnc_impl.cc
    lib/ax25/ax25_protocol.c
    lib/ax25/kiss_protocol.c
    lib/fx25/fx25_encoder_impl.cc
    lib/fx25/fx25_decoder_impl.cc
    lib/fx25/fx25_fec_impl.cc
    lib/fx25/fx25_protocol.c
    lib/il2p/il2p_encoder_impl.cc
    lib/il2p/il2p_decoder_impl.cc
    lib/il2p/il2p_reed_solomon_impl.cc
    lib/il2p/il2p_protocol.c
)

# Create the library
add_library(gnuradio-packet_protocols SHARED ${GR_PACKET_PROTOCOLS_SOURCES})

# Link libraries
target_link_libraries(gnuradio-packet_protocols
    ${GNURADIO_LIBRARIES}
)

# Set target properties
set_target_properties(gnuradio-packet_protocols PROPERTIES
    VERSION ${GR_PACKET_PROTOCOLS_VERSION}
    SOVERSION ${GR_PACKET_PROTOCOLS_VERSION_MAJOR}
)

# Install the library
install(TARGETS gnuradio-packet_protocols
    DESTINATION ${GR_LIBRARY_DIR}
)

# Install headers
install(DIRECTORY include/gnuradio/packet_protocols/
    DESTINATION ${GR_INCLUDE_DIR}/gnuradio/packet_protocols
    FILES_MATCHING PATTERN "*.h"
)

# Install Python bindings
install(DIRECTORY python/
    DESTINATION ${GR_PYTHON_DIR}/gnuradio/packet_protocols
    FILES_MATCHING PATTERN "*.py"
)

# Install GRC blocks (YAML format for GNU Radio 3.10+)
# Install files directly to avoid nested directory structure
install(FILES
    grc/ax25_decoder.block.yml
    grc/ax25_encoder.block.yml
    grc/fx25_decoder.block.yml
    grc/fx25_encoder.block.yml
    grc/il2p_decoder.block.yml
    grc/il2p_encoder.block.yml
    grc/kiss_tnc.block.yml
    DESTINATION ${GR_PKG_DATA_DIR}/grc/blocks
)

# Install examples
install(DIRECTORY examples/
    DESTINATION ${GR_PKG_DATA_DIR}/gr-packet-protocols/examples
    FILES_MATCHING PATTERN "*.grc"
)

# Install documentation
install(DIRECTORY docs/
    DESTINATION ${GR_PKG_DATA_DIR}/gr-packet-protocols/docs
    FILES_MATCHING PATTERN "*.md"
)

# Python bindings - enabled by default
option(ENABLE_PYTHON "Enable Python bindings" ON)

if(ENABLE_PYTHON)
    find_package(Python3 COMPONENTS Interpreter Development)
    find_package(pybind11)
    
    if(Python3_FOUND AND pybind11_FOUND)
        pybind11_add_module(packet_protocols_python
            python/packet_protocols_bindings.cc
        )
        
        target_link_libraries(packet_protocols_python PRIVATE
            gnuradio-packet_protocols
            ${GNURADIO_LIBRARIES}
        )
        
        install(TARGETS packet_protocols_python
            DESTINATION ${GR_PYTHON_DIR}/gnuradio/packet_protocols
        )
    endif()
endif()

# Testing
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
if(ENABLE_DOXYGEN)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    endif()
endif()

# Uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    COMMENT "Uninstalling gr-packet-protocols"
)

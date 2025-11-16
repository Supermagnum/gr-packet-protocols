########################################################################
# Testing directory
########################################################################

include(CTest)

# Simple test that doesn't require CppUnit
add_executable(test_simple test_simple.cc)

# Find fmt library
find_library(FMT_LIBRARY fmt)
if(FMT_LIBRARY)
    target_link_libraries(test_simple ${FMT_LIBRARY})
endif()

target_link_libraries(test_simple
    gnuradio-packet_protocols
    ${GNURADIO_LIBRARIES}
)

# Ensure test can find the library at runtime
set_target_properties(test_simple PROPERTIES
    INSTALL_RPATH "${CMAKE_CURRENT_BINARY_DIR}/.."
    BUILD_WITH_INSTALL_RPATH TRUE
)

add_test(NAME test_simple COMMAND test_simple)

# CppUnit-based tests (optional)
find_package(CppUnit)

if(CPPUNIT_FOUND)
    include_directories(${CPPUNIT_INCLUDE_DIRS})
    
    set(GR_PACKET_PROTOCOLS_TEST_SOURCES
        test_block_instantiation.cc
    )
    
    add_executable(qa_block_instantiation ${GR_PACKET_PROTOCOLS_TEST_SOURCES})
    
    target_link_libraries(qa_block_instantiation
        gnuradio-packet_protocols
        ${GNURADIO_LIBRARIES}
        ${CPPUNIT_LIBRARIES}
    )
    
    add_test(NAME qa_block_instantiation COMMAND qa_block_instantiation)
    
else()
    message(STATUS "CppUnit not found, skipping CppUnit-based tests")
    message(STATUS "Install cppunit-dev to enable CppUnit unit tests")
    message(STATUS "Basic tests will still run without CppUnit")
endif()

# Python tests (if Python bindings are enabled)
if(ENABLE_PYTHON AND Python3_FOUND)
    find_program(PYTHON3_EXECUTABLE python3)
    if(PYTHON3_EXECUTABLE)
        add_test(NAME test_python_import
            COMMAND ${PYTHON3_EXECUTABLE} -c "import sys; sys.path.insert(0, '${CMAKE_CURRENT_BINARY_DIR}'); import packet_protocols_python; print('Python bindings import successful')"
        )
        set_tests_properties(test_python_import PROPERTIES
            ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_CURRENT_BINARY_DIR}:$ENV{LD_LIBRARY_PATH};PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}:$ENV{PYTHONPATH}"
        )
    endif()
endif()


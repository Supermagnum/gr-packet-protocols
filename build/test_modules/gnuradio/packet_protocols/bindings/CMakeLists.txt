# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT packet_protocols_sources)
    message(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT packet_protocols_sources)

########################################################################
# Check for pygccxml
########################################################################
gr_python_check_module_raw("pygccxml" "import pygccxml" PYGCCXML_FOUND)

include(GrPybind)

########################################################################
# Python Bindings
########################################################################

list(APPEND packet_protocols_python_files
    ax25_encoder_python.cc
    ax25_decoder_python.cc
    fx25_encoder_python.cc
    fx25_decoder_python.cc
    il2p_encoder_python.cc
    il2p_decoder_python.cc
    kiss_tnc_python.cc python_bindings.cc)

gr_pybind_make_oot(packet_protocols ../../.. gr::packet_protocols "${packet_protocols_python_files}")

# copy bindings extension for use in QA test module
add_custom_command(
    TARGET packet_protocols_python
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:packet_protocols_python>
            ${PROJECT_BINARY_DIR}/test_modules/gnuradio/packet_protocols/)

install(
    TARGETS packet_protocols_python
    DESTINATION ${GR_PYTHON_DIR}/gnuradio/packet_protocols
    COMPONENT pythonapi)

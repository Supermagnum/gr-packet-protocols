# Install script for directory: /home/haaken/github-projects/gr-packet-protocols/grc

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/gnuradio/grc/blocks" TYPE FILE FILES
    "/home/haaken/github-projects/gr-packet-protocols/grc/packet_protocols_ax25_encoder.block.yml"
    "/home/haaken/github-projects/gr-packet-protocols/grc/packet_protocols_ax25_decoder.block.yml"
    "/home/haaken/github-projects/gr-packet-protocols/grc/packet_protocols_fx25_encoder.block.yml"
    "/home/haaken/github-projects/gr-packet-protocols/grc/packet_protocols_fx25_decoder.block.yml"
    "/home/haaken/github-projects/gr-packet-protocols/grc/packet_protocols_il2p_encoder.block.yml"
    "/home/haaken/github-projects/gr-packet-protocols/grc/packet_protocols_il2p_decoder.block.yml"
    "/home/haaken/github-projects/gr-packet-protocols/grc/packet_protocols_kiss_tnc.block.yml"
    "/home/haaken/github-projects/gr-packet-protocols/grc/packet_protocols_link_quality_monitor.block.yml"
    "/home/haaken/github-projects/gr-packet-protocols/grc/packet_protocols_adaptive_rate_control.block.yml"
    "/home/haaken/github-projects/gr-packet-protocols/grc/packet_protocols_modulation_negotiation.block.yml"
    "/home/haaken/github-projects/gr-packet-protocols/grc/packet_protocols_adaptive_modulator.block.yml"
    "/home/haaken/github-projects/gr-packet-protocols/grc/packet_protocols_modulation_switch.block.yml"
    )
endif()


# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.22)
project(
  gr-packet-protocols4
  VERSION 0.1.0
  LANGUAGES C CXX
  DESCRIPTION "GNU Radio 4.0 out-of-tree AX.25 / FX.25 / IL2P packet protocols")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GR_PACKET_PROTOCOLS4_BUILD_TESTS "Build Boost.UT tests" ON)

find_package(gnuradio4 REQUIRED CONFIG)

add_library(gnuradio4-packet-protocols-ax25 STATIC third_party/ax25/ax25_protocol.c)
add_library(gnuradio4::packet-protocols-ax25 ALIAS gnuradio4-packet-protocols-ax25)
target_include_directories(
  gnuradio4-packet-protocols-ax25
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/ax25>"
         "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
         "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

add_library(gnuradio4-gr-packet-protocols INTERFACE)
add_library(gnuradio4::gr-packet-protocols ALIAS gnuradio4-gr-packet-protocols)
target_include_directories(
  gnuradio4-gr-packet-protocols
  INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(gnuradio4-gr-packet-protocols INTERFACE gnuradio4::gnuradio-core gnuradio4::gnuradio-blocklib-core
                                                                 gnuradio4-packet-protocols-ax25)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(
  TARGETS gnuradio4-gr-packet-protocols gnuradio4-packet-protocols-ax25
  EXPORT gr-packet-protocols4-targets
  INCLUDES
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(
  EXPORT gr-packet-protocols4-targets
  FILE gr-packet-protocols4-targets.cmake
  NAMESPACE gnuradio4::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/gr-packet-protocols4")

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gr-packet-protocols4-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/gr-packet-protocols4-config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/gr-packet-protocols4")
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/gr-packet-protocols4-config-version.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gr-packet-protocols4-config.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/gr-packet-protocols4-config-version.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/gr-packet-protocols4")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/gnuradio4-gr-packet-protocols.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/gnuradio4-gr-packet-protocols.pc"
             @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gnuradio4-gr-packet-protocols.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

if(GR_PACKET_PROTOCOLS4_BUILD_TESTS)
  include(FetchContent)
  set(BOOST_UT_DISABLE_MODULE ON CACHE BOOL "Disable UT Module Support" FORCE)
  FetchContent_Declare(
    ut
    GIT_REPOSITORY https://github.com/boost-ext/ut.git
    GIT_TAG 53e17f25119598c6458d30351b260193096ba67e
    EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(ut)
  enable_testing()
  add_subdirectory(test)
endif()

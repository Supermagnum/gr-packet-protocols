find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_PACKET_PROTOCOLS gnuradio-packet_protocols)

FIND_PATH(
    GR_PACKET_PROTOCOLS_INCLUDE_DIRS
    NAMES gnuradio/packet_protocols/api.h
    HINTS $ENV{PACKET_PROTOCOLS_DIR}/include
        ${PC_PACKET_PROTOCOLS_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_PACKET_PROTOCOLS_LIBRARIES
    NAMES gnuradio-packet_protocols
    HINTS $ENV{PACKET_PROTOCOLS_DIR}/lib
        ${PC_PACKET_PROTOCOLS_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-packet_protocolsTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_PACKET_PROTOCOLS DEFAULT_MSG GR_PACKET_PROTOCOLS_LIBRARIES GR_PACKET_PROTOCOLS_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_PACKET_PROTOCOLS_LIBRARIES GR_PACKET_PROTOCOLS_INCLUDE_DIRS)

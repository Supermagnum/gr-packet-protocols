#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "gnuradio::gnuradio-packet_protocols" for configuration "Release"
set_property(TARGET gnuradio::gnuradio-packet_protocols APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(gnuradio::gnuradio-packet_protocols PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libgnuradio-packet_protocols.so.1.0.0.0"
  IMPORTED_SONAME_RELEASE "libgnuradio-packet_protocols.so.1.0.0"
  )

list(APPEND _cmake_import_check_targets gnuradio::gnuradio-packet_protocols )
list(APPEND _cmake_import_check_files_for_gnuradio::gnuradio-packet_protocols "${_IMPORT_PREFIX}/lib/x86_64-linux-gnu/libgnuradio-packet_protocols.so.1.0.0.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

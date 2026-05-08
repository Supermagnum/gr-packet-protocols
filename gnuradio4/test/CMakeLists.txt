function(gr_packet_protocols4_add_ut src)
  get_filename_component(_name "${src}" NAME_WE)
  add_executable("${_name}" "${src}")
  target_link_libraries(
    "${_name}"
    PRIVATE gnuradio4::gnuradio-options gnuradio4::gnuradio-core gnuradio4::gnuradio-blocklib-core gnuradio4::gr-packet-protocols ut)
  add_test(NAME "${_name}" COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:${_name}>)
endfunction()

gr_packet_protocols4_add_ut(qa_Ax25Encoder.cpp)
gr_packet_protocols4_add_ut(qa_Ax25Decoder.cpp)
gr_packet_protocols4_add_ut(qa_Fx25Encoder.cpp)
gr_packet_protocols4_add_ut(qa_Fx25Decoder.cpp)
gr_packet_protocols4_add_ut(qa_Il2pEncoder.cpp)
gr_packet_protocols4_add_ut(qa_Il2pDecoder.cpp)
gr_packet_protocols4_add_ut(qa_KissTnc.cpp)
gr_packet_protocols4_add_ut(qa_LinkQualityMonitor.cpp)
gr_packet_protocols4_add_ut(qa_AdaptiveRateControl.cpp)

add_executable(qa_Ax25Protocol qa_Ax25Protocol.cpp)
target_include_directories(qa_Ax25Protocol PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(qa_Ax25Protocol PRIVATE gnuradio4::packet-protocols-ax25 ut)
add_test(NAME qa_Ax25Protocol COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:qa_Ax25Protocol>)

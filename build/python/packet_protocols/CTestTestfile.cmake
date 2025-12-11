# CMake generated Testfile for 
# Source directory: /home/haaken/github-projects/gr-packet-protocols/python/packet_protocols
# Build directory: /home/haaken/github-projects/gr-packet-protocols/build/python/packet_protocols
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(qa_ax25_encoder "/usr/bin/sh" "qa_ax25_encoder_test.sh")
set_tests_properties(qa_ax25_encoder PROPERTIES  _BACKTRACE_TRIPLES "/usr/lib/x86_64-linux-gnu/cmake/gnuradio/GrTest.cmake;119;add_test;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;41;GR_ADD_TEST;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;0;")
add_test(qa_ax25_decoder "/usr/bin/sh" "qa_ax25_decoder_test.sh")
set_tests_properties(qa_ax25_decoder PROPERTIES  _BACKTRACE_TRIPLES "/usr/lib/x86_64-linux-gnu/cmake/gnuradio/GrTest.cmake;119;add_test;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;42;GR_ADD_TEST;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;0;")
add_test(qa_fx25_encoder "/usr/bin/sh" "qa_fx25_encoder_test.sh")
set_tests_properties(qa_fx25_encoder PROPERTIES  _BACKTRACE_TRIPLES "/usr/lib/x86_64-linux-gnu/cmake/gnuradio/GrTest.cmake;119;add_test;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;43;GR_ADD_TEST;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;0;")
add_test(qa_fx25_decoder "/usr/bin/sh" "qa_fx25_decoder_test.sh")
set_tests_properties(qa_fx25_decoder PROPERTIES  _BACKTRACE_TRIPLES "/usr/lib/x86_64-linux-gnu/cmake/gnuradio/GrTest.cmake;119;add_test;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;44;GR_ADD_TEST;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;0;")
add_test(qa_il2p_encoder "/usr/bin/sh" "qa_il2p_encoder_test.sh")
set_tests_properties(qa_il2p_encoder PROPERTIES  _BACKTRACE_TRIPLES "/usr/lib/x86_64-linux-gnu/cmake/gnuradio/GrTest.cmake;119;add_test;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;45;GR_ADD_TEST;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;0;")
add_test(qa_il2p_decoder "/usr/bin/sh" "qa_il2p_decoder_test.sh")
set_tests_properties(qa_il2p_decoder PROPERTIES  _BACKTRACE_TRIPLES "/usr/lib/x86_64-linux-gnu/cmake/gnuradio/GrTest.cmake;119;add_test;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;46;GR_ADD_TEST;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;0;")
add_test(qa_kiss_tnc "/usr/bin/sh" "qa_kiss_tnc_test.sh")
set_tests_properties(qa_kiss_tnc PROPERTIES  _BACKTRACE_TRIPLES "/usr/lib/x86_64-linux-gnu/cmake/gnuradio/GrTest.cmake;119;add_test;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;47;GR_ADD_TEST;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;0;")
add_test(qa_link_quality_monitor "/usr/bin/sh" "qa_link_quality_monitor_test.sh")
set_tests_properties(qa_link_quality_monitor PROPERTIES  _BACKTRACE_TRIPLES "/usr/lib/x86_64-linux-gnu/cmake/gnuradio/GrTest.cmake;119;add_test;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;48;GR_ADD_TEST;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;0;")
add_test(qa_adaptive_rate_control "/usr/bin/sh" "qa_adaptive_rate_control_test.sh")
set_tests_properties(qa_adaptive_rate_control PROPERTIES  _BACKTRACE_TRIPLES "/usr/lib/x86_64-linux-gnu/cmake/gnuradio/GrTest.cmake;119;add_test;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;49;GR_ADD_TEST;/home/haaken/github-projects/gr-packet-protocols/python/packet_protocols/CMakeLists.txt;0;")
subdirs("bindings")

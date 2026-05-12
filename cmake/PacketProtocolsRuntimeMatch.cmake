# SPDX-License-Identifier: GPL-3.0-or-later

function(gr_packet_protocols_verify_gnuradio_headers_vs_python _project_root _python_exe _gnuradio_imported_target _allow_mismatch)
  if(NOT TARGET "${_gnuradio_imported_target}")
    return()
  endif()
  get_target_property(_inc_dirs "${_gnuradio_imported_target}" INTERFACE_INCLUDE_DIRECTORIES)
  if(NOT _inc_dirs)
    message(WARNING "gr-packet-protocols: INTERFACE_INCLUDE_DIRECTORIES missing on ${_gnuradio_imported_target}; skipping runtime/header match.")
    return()
  endif()
  if(_inc_dirs MATCHES "\\\$<")
    message(WARNING "gr-packet-protocols: generator-expression include dirs on ${_gnuradio_imported_target}; skipping runtime/header match.")
    return()
  endif()
  list(GET _inc_dirs 0 _inc0)
  execute_process(
    COMMAND "${_python_exe}" "${_project_root}/cmake/gr_python_matches_headers.py" "${_inc0}"
    RESULT_VARIABLE _rc
    OUTPUT_VARIABLE _out
    ERROR_VARIABLE _err)
  if(NOT _rc EQUAL 0)
    set(_msg "${_err}${_out}")
    if(_allow_mismatch)
      message(WARNING "gr-packet-protocols: GNU Radio header/Python runtime check failed but mismatch override is enabled:\n${_msg}")
    else()
      message(SEND_ERROR "gr-packet-protocols: GNU Radio header/Python runtime check failed:\n${_msg}")
    endif()
  endif()
endfunction()

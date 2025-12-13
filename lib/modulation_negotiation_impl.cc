/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "modulation_negotiation_impl.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/message.h>
#include <pmt/pmt.h>
#include <gnuradio/packet_protocols/common.h>
#include <algorithm>
#include <chrono>
#include <cstring>

namespace gr {
namespace packet_protocols {

modulation_negotiation::sptr modulation_negotiation::make(
    const std::string& station_id, const std::vector<modulation_mode_t>& supported_modes,
    int negotiation_timeout_ms) {
  return gnuradio::make_block_sptr<modulation_negotiation_impl>(
      station_id, supported_modes, negotiation_timeout_ms);
}

modulation_negotiation_impl::modulation_negotiation_impl(
    const std::string& station_id, const std::vector<modulation_mode_t>& supported_modes,
    int negotiation_timeout_ms)
    : gr::sync_block("modulation_negotiation",
                     gr::io_signature::make(1, 1, sizeof(char)),
                     gr::io_signature::make(1, 1, sizeof(char))),
      d_station_id(station_id),
      d_supported_modes(supported_modes),
      d_negotiation_timeout_ms(negotiation_timeout_ms),
      d_negotiating(false),
      d_negotiated_mode(modulation_mode_t::MODE_4FSK),
      d_pending_mode(modulation_mode_t::MODE_4FSK),
      d_send_kiss_frame_callback(nullptr),
      d_auto_negotiation_enabled(false),
      d_rate_control(nullptr),
      d_last_monitored_mode(modulation_mode_t::MODE_4FSK) {
  // Default to first supported mode
  if (!d_supported_modes.empty()) {
    d_negotiated_mode = d_supported_modes[0];
    d_pending_mode = d_supported_modes[0];
  }
  
  // Register message port for receiving negotiation frames
  message_port_register_in(pmt::mp("negotiation_in"));
  set_msg_handler(pmt::mp("negotiation_in"),
                  [this](pmt::pmt_t msg) {
                    if (pmt::is_pair(msg)) {
                      pmt::pmt_t command_pmt = pmt::car(msg);
                      pmt::pmt_t data_pmt = pmt::cdr(msg);
                      if (pmt::is_integer(command_pmt) && pmt::is_u8vector(data_pmt)) {
                        uint8_t cmd = static_cast<uint8_t>(pmt::to_long(command_pmt));
                        size_t len;
                        const uint8_t* data = pmt::u8vector_elements(data_pmt, len);
                        handle_negotiation_frame(cmd, data, len);
                      }
                    }
                  });
}

modulation_negotiation_impl::~modulation_negotiation_impl() {}

int modulation_negotiation_impl::work(int noutput_items,
                                       gr_vector_const_void_star& input_items,
                                       gr_vector_void_star& output_items) {
  const char* in = (const char*)input_items[0];
  char* out = (char*)output_items[0];

  // Pass through data
  memcpy(out, in, noutput_items * sizeof(char));

  // Check for negotiation timeout and automatic negotiation
  std::lock_guard<std::mutex> lock(d_mutex);
  if (d_negotiating) {
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now.time_since_epoch())
                       .count() -
                   d_negotiation_start_time;
      if (elapsed > static_cast<long long>(d_negotiation_timeout_ms)) {
      // Negotiation timeout - revert to previous mode
      d_negotiating = false;
      d_pending_mode = d_negotiated_mode;
    }
  }
  
  // Check for automatic negotiation triggers
  if (d_auto_negotiation_enabled && d_rate_control && !d_negotiating) {
    check_mode_change();
  }

  return noutput_items;
}

void modulation_negotiation_impl::initiate_negotiation(
    const std::string& remote_station_id, modulation_mode_t proposed_mode) {
  std::lock_guard<std::mutex> lock(d_mutex);

  // Check if proposed mode is supported
  bool mode_supported =
      std::find(d_supported_modes.begin(), d_supported_modes.end(), proposed_mode) !=
      d_supported_modes.end();

  if (!mode_supported) {
    // Use default mode if proposed is not supported
    proposed_mode = d_negotiated_mode;
  }

  d_remote_station_id = remote_station_id;
  d_pending_mode = proposed_mode;
  d_negotiating = true;
  d_negotiation_start_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count();

  // Encode and send negotiation request frame
  std::vector<uint8_t> frame_data = encode_negotiation_request(
      d_station_id, proposed_mode, d_supported_modes);
  send_kiss_frame(KISS_CMD_NEG_REQ, frame_data);
}

modulation_mode_t modulation_negotiation_impl::get_negotiated_mode() const {
  std::lock_guard<std::mutex> lock(d_mutex);
  return d_negotiated_mode;
}

bool modulation_negotiation_impl::is_negotiating() const {
  std::lock_guard<std::mutex> lock(d_mutex);
  return d_negotiating;
}

void modulation_negotiation_impl::send_quality_feedback(
    const std::string& remote_station_id, float snr_db, float ber,
    float quality_score) {
  // Encode and send quality feedback frame
  std::vector<uint8_t> frame_data = encode_quality_feedback(
      d_station_id, snr_db, ber, quality_score);
  send_kiss_frame(KISS_CMD_QUALITY_FB, frame_data);
}

void modulation_negotiation_impl::handle_negotiation_frame(uint8_t command,
                                                            const uint8_t* data, size_t length) {
  std::lock_guard<std::mutex> lock(d_mutex);
  
  switch (command) {
    case KISS_CMD_NEG_REQ: {
      // Received negotiation request
      std::string remote_id;
      modulation_mode_t proposed_mode;
      std::vector<modulation_mode_t> remote_supported_modes;
      
      if (decode_negotiation_request(data, length, remote_id, proposed_mode,
                                     remote_supported_modes)) {
        // Check if proposed mode is supported
        bool mode_supported = std::find(d_supported_modes.begin(), d_supported_modes.end(),
                                        proposed_mode) != d_supported_modes.end();
        
        if (mode_supported) {
          // Accept negotiation
          d_negotiated_mode = proposed_mode;
          d_negotiated_modes[remote_id] = proposed_mode;
          
          // Send response
          std::vector<uint8_t> resp_data = encode_negotiation_response(
              d_station_id, true, proposed_mode);
          send_kiss_frame(KISS_CMD_NEG_RESP, resp_data);
        } else {
          // Reject - find common mode
          modulation_mode_t common_mode = d_negotiated_mode;
          for (auto mode : remote_supported_modes) {
            if (std::find(d_supported_modes.begin(), d_supported_modes.end(), mode) !=
                d_supported_modes.end()) {
              common_mode = mode;
              break;
            }
          }
          
          // Send response with common mode
          std::vector<uint8_t> resp_data = encode_negotiation_response(
              d_station_id, true, common_mode);
          send_kiss_frame(KISS_CMD_NEG_RESP, resp_data);
          
          d_negotiated_mode = common_mode;
          d_negotiated_modes[remote_id] = common_mode;
        }
      }
      break;
    }
    
    case KISS_CMD_NEG_RESP: {
      // Received negotiation response
      std::string remote_id;
      bool accepted;
      modulation_mode_t negotiated_mode;
      
      if (decode_negotiation_response(data, length, remote_id, accepted, negotiated_mode)) {
        if (accepted) {
          d_negotiated_mode = negotiated_mode;
          d_negotiated_modes[remote_id] = negotiated_mode;
          d_negotiating = false;
          
          // Send acknowledgment
          std::vector<uint8_t> ack_data = encode_mode_change(d_station_id, negotiated_mode);
          send_kiss_frame(KISS_CMD_NEG_ACK, ack_data);
        } else {
          // Negotiation rejected
          d_negotiating = false;
          d_pending_mode = d_negotiated_mode;
        }
      }
      break;
    }
    
    case KISS_CMD_MODE_CHANGE: {
      // Received mode change notification
      std::string remote_id;
      modulation_mode_t new_mode;
      
      if (decode_mode_change(data, length, remote_id, new_mode)) {
        d_negotiated_modes[remote_id] = new_mode;
      }
      break;
    }
    
    case KISS_CMD_QUALITY_FB: {
      // Received quality feedback (can be used for future enhancements)
      std::string remote_id;
      float snr_db, ber, quality_score;
      
      if (decode_quality_feedback(data, length, remote_id, snr_db, ber, quality_score)) {
        // Store quality feedback for future use
        // Could be used for coordinated adaptation
      }
      break;
    }
    
    default:
      break;
  }
}

void modulation_negotiation_impl::set_kiss_frame_sender(
    std::function<void(uint8_t, const std::vector<uint8_t>&)> callback) {
  std::lock_guard<std::mutex> lock(d_mutex);
  d_send_kiss_frame_callback = callback;
}

void modulation_negotiation_impl::send_kiss_frame(uint8_t command,
                                                   const std::vector<uint8_t>& data) {
  if (d_send_kiss_frame_callback) {
    d_send_kiss_frame_callback(command, data);
  }
}

void modulation_negotiation_impl::set_auto_negotiation_enabled(bool enabled,
                                                                 adaptive_rate_control* rate_control) {
  std::lock_guard<std::mutex> lock(d_mutex);
  d_auto_negotiation_enabled = enabled;
  d_rate_control = rate_control;
  if (rate_control) {
    d_last_monitored_mode = rate_control->get_modulation_mode();
  }
}

void modulation_negotiation_impl::check_mode_change() {
  if (!d_rate_control) {
    return;
  }
  
  modulation_mode_t current_mode = d_rate_control->get_modulation_mode();
  if (current_mode != d_last_monitored_mode) {
    // Mode changed - initiate negotiation with all known remote stations
    d_last_monitored_mode = current_mode;
    
    // Send mode change notification to all negotiated stations
    for (const auto& pair : d_negotiated_modes) {
      std::vector<uint8_t> frame_data = encode_mode_change(d_station_id, current_mode);
      send_kiss_frame(KISS_CMD_MODE_CHANGE, frame_data);
    }
    
    // If we have an active remote station, negotiate with it
    if (!d_remote_station_id.empty()) {
      initiate_negotiation(d_remote_station_id, current_mode);
    }
  }
}

std::vector<modulation_mode_t> modulation_negotiation_impl::get_supported_modes() const {
  std::lock_guard<std::mutex> lock(d_mutex);
  return d_supported_modes;
}

} // namespace packet_protocols
} // namespace gr


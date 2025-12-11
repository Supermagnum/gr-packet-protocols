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
      d_pending_mode(modulation_mode_t::MODE_4FSK) {
  // Default to first supported mode
  if (!d_supported_modes.empty()) {
    d_negotiated_mode = d_supported_modes[0];
    d_pending_mode = d_supported_modes[0];
  }
}

modulation_negotiation_impl::~modulation_negotiation_impl() {}

int modulation_negotiation_impl::work(int noutput_items,
                                       gr_vector_const_void_star& input_items,
                                       gr_vector_void_star& output_items) {
  const char* in = (const char*)input_items[0];
  char* out = (char*)output_items[0];

  // Pass through data
  memcpy(out, in, noutput_items * sizeof(char));

  // Check for negotiation timeout
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

  // In a full implementation, this would send a KISS negotiation frame
  // For now, we'll accept the negotiation immediately if mode is supported
  if (mode_supported) {
    d_negotiated_mode = proposed_mode;
    d_negotiated_modes[remote_station_id] = proposed_mode;
  }
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
  // In a full implementation, this would send a KISS quality feedback frame
  // For now, this is a placeholder
  (void)remote_station_id;
  (void)snr_db;
  (void)ber;
  (void)quality_score;
}

std::vector<modulation_mode_t> modulation_negotiation_impl::get_supported_modes() const {
  std::lock_guard<std::mutex> lock(d_mutex);
  return d_supported_modes;
}

} // namespace packet_protocols
} // namespace gr


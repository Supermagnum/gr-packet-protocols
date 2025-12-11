/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "adaptive_rate_control_impl.h"
#include <gnuradio/io_signature.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace gr {
namespace packet_protocols {

adaptive_rate_control::sptr adaptive_rate_control::make(modulation_mode_t initial_mode,
                                                          bool enable_adaptation,
                                                          float hysteresis_db) {
  return gnuradio::make_block_sptr<adaptive_rate_control_impl>(
      initial_mode, enable_adaptation, hysteresis_db);
}

adaptive_rate_control_impl::adaptive_rate_control_impl(modulation_mode_t initial_mode,
                                                       bool enable_adaptation,
                                                       float hysteresis_db)
    : gr::sync_block("adaptive_rate_control",
                     gr::io_signature::make(1, 1, sizeof(char)),
                     gr::io_signature::make(1, 1, sizeof(char))),
      d_current_mode(initial_mode),
      d_adaptation_enabled(enable_adaptation),
      d_hysteresis_db(hysteresis_db),
      d_last_snr_db(0.0f),
      d_last_mode(initial_mode) {
  initialize_thresholds();
}

adaptive_rate_control_impl::~adaptive_rate_control_impl() {}

void adaptive_rate_control_impl::initialize_thresholds() {
  // Define thresholds and data rates for each modulation mode
  // Lower order modulations are more robust but slower
  // Higher order modulations are faster but require better SNR

  // 2FSK - Most robust, lowest rate
  d_mode_thresholds[modulation_mode_t::MODE_2FSK] = {0.0f, 15.0f, 0.01f, 0.3f};
  d_mode_data_rates[modulation_mode_t::MODE_2FSK] = 1200;

  // 4FSK - Good balance
  d_mode_thresholds[modulation_mode_t::MODE_4FSK] = {8.0f, 20.0f, 0.005f, 0.5f};
  d_mode_data_rates[modulation_mode_t::MODE_4FSK] = 2400;

  // 8FSK - Higher rate, needs better SNR
  d_mode_thresholds[modulation_mode_t::MODE_8FSK] = {12.0f, 25.0f, 0.001f, 0.7f};
  d_mode_data_rates[modulation_mode_t::MODE_8FSK] = 3600;

  // 16FSK - Highest FSK rate
  d_mode_thresholds[modulation_mode_t::MODE_16FSK] = {18.0f, 30.0f, 0.0005f, 0.8f};
  d_mode_data_rates[modulation_mode_t::MODE_16FSK] = 4800;

  // BPSK - Robust PSK
  d_mode_thresholds[modulation_mode_t::MODE_BPSK] = {6.0f, 18.0f, 0.01f, 0.4f};
  d_mode_data_rates[modulation_mode_t::MODE_BPSK] = 1200;

  // QPSK - Good balance for PSK
  d_mode_thresholds[modulation_mode_t::MODE_QPSK] = {10.0f, 22.0f, 0.005f, 0.6f};
  d_mode_data_rates[modulation_mode_t::MODE_QPSK] = 2400;

  // 8PSK - Higher rate PSK
  d_mode_thresholds[modulation_mode_t::MODE_8PSK] = {14.0f, 26.0f, 0.001f, 0.75f};
  d_mode_data_rates[modulation_mode_t::MODE_8PSK] = 3600;

  // 16-QAM - High rate, needs good SNR
  d_mode_thresholds[modulation_mode_t::MODE_QAM16] = {16.0f, 28.0f, 0.0005f, 0.8f};
  d_mode_data_rates[modulation_mode_t::MODE_QAM16] = 4800;

  // 64-QAM - Highest rate, needs excellent SNR
  d_mode_thresholds[modulation_mode_t::MODE_QAM64] = {22.0f, 35.0f, 0.0001f, 0.9f};
  d_mode_data_rates[modulation_mode_t::MODE_QAM64] = 9600;
}

int adaptive_rate_control_impl::work(int noutput_items,
                                     gr_vector_const_void_star& input_items,
                                     gr_vector_void_star& output_items) {
  const char* in = (const char*)input_items[0];
  char* out = (char*)output_items[0];

  // Pass through data
  memcpy(out, in, noutput_items * sizeof(char));

  return noutput_items;
}

modulation_mode_t adaptive_rate_control_impl::get_modulation_mode() const {
  std::lock_guard<std::mutex> lock(d_mutex);
  return d_current_mode;
}

void adaptive_rate_control_impl::set_modulation_mode(modulation_mode_t mode) {
  std::lock_guard<std::mutex> lock(d_mutex);
  d_current_mode = mode;
  d_last_mode = mode;
}

void adaptive_rate_control_impl::set_adaptation_enabled(bool enabled) {
  std::lock_guard<std::mutex> lock(d_mutex);
  d_adaptation_enabled = enabled;
}

void adaptive_rate_control_impl::update_quality(float snr_db, float ber,
                                                  float quality_score) {
  if (!d_adaptation_enabled) {
    return;
  }

  std::lock_guard<std::mutex> lock(d_mutex);

  d_last_snr_db = snr_db;

  // Get current mode thresholds
  auto current_thresholds = get_thresholds(d_current_mode);

  // Check if we should switch to a higher rate mode
  if (snr_db > (current_thresholds.snr_max_dB + d_hysteresis_db) &&
      ber < current_thresholds.ber_max && quality_score > current_thresholds.quality_min) {
    // Try to find a higher rate mode
    modulation_mode_t recommended = recommend_mode(snr_db, ber);
    if (recommended != d_current_mode) {
      d_last_mode = d_current_mode;
      d_current_mode = recommended;
    }
  }
  // Check if we should switch to a lower rate mode (more robust)
  else if (snr_db < (current_thresholds.snr_min_dB - d_hysteresis_db) ||
           ber > current_thresholds.ber_max ||
           quality_score < (current_thresholds.quality_min - 0.2f)) {
    // Try to find a more robust mode
    modulation_mode_t recommended = recommend_mode(snr_db, ber);
    if (recommended != d_current_mode) {
      d_last_mode = d_current_mode;
      d_current_mode = recommended;
    }
  }
}

modulation_mode_t adaptive_rate_control_impl::recommend_mode(float snr_db,
                                                               float ber) const {
  // Find the highest rate mode that meets the quality requirements
  modulation_mode_t best_mode = modulation_mode_t::MODE_2FSK;
  int best_rate = 0;

  // Check modes in order from highest to lowest rate
  std::vector<modulation_mode_t> modes = {
      modulation_mode_t::MODE_QAM64, modulation_mode_t::MODE_QAM16,
      modulation_mode_t::MODE_16FSK, modulation_mode_t::MODE_8PSK,
      modulation_mode_t::MODE_8FSK,  modulation_mode_t::MODE_QPSK,
      modulation_mode_t::MODE_4FSK,  modulation_mode_t::MODE_BPSK,
      modulation_mode_t::MODE_2FSK};

  for (auto mode : modes) {
    auto thresholds = get_thresholds(mode);
    auto rate_it = d_mode_data_rates.find(mode);

    if (rate_it == d_mode_data_rates.end()) {
      continue;
    }

    // Check if this mode meets requirements
    if (snr_db >= thresholds.snr_min_dB && snr_db <= thresholds.snr_max_dB &&
        ber <= thresholds.ber_max) {
      // Prefer higher rate if multiple modes meet requirements
      if (rate_it->second > best_rate) {
        best_mode = mode;
        best_rate = rate_it->second;
      }
    }
  }

  return best_mode;
}

int adaptive_rate_control_impl::get_data_rate() const {
  std::lock_guard<std::mutex> lock(d_mutex);
  auto it = d_mode_data_rates.find(d_current_mode);
  if (it != d_mode_data_rates.end()) {
    return it->second;
  }
  return 1200;  // Default rate
}

rate_thresholds_t adaptive_rate_control_impl::get_thresholds(
    modulation_mode_t mode) const {
  auto it = d_mode_thresholds.find(mode);
  if (it != d_mode_thresholds.end()) {
    return it->second;
  }
  // Default thresholds
  return {0.0f, 15.0f, 0.01f, 0.3f};
}

} // namespace packet_protocols
} // namespace gr


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
                                                          float hysteresis_db,
                                                          bool enable_tier4) {
  return gnuradio::make_block_sptr<adaptive_rate_control_impl>(
      initial_mode, enable_adaptation, hysteresis_db, enable_tier4);
}

adaptive_rate_control_impl::adaptive_rate_control_impl(modulation_mode_t initial_mode,
                                                       bool enable_adaptation,
                                                       float hysteresis_db,
                                                       bool enable_tier4)
    : gr::sync_block("adaptive_rate_control",
                     gr::io_signature::make(1, 1, sizeof(char)),
                     gr::io_signature::make(1, 1, sizeof(char))),
      d_current_mode(initial_mode),
      d_adaptation_enabled(enable_adaptation),
      d_hysteresis_db(hysteresis_db),
      d_enable_tier4(enable_tier4),
      d_last_snr_db(0.0f),
      d_last_mode(initial_mode) {
  // Validate initial mode - reject Tier 4 if disabled
  if (is_tier4_mode(initial_mode) && !d_enable_tier4) {
    d_current_mode = modulation_mode_t::MODE_2FSK;  // Fallback to default
  }
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

  // 16-QAM @ 2,400 baud (2,400 × 4 = 9,600 bps)
  d_mode_thresholds[modulation_mode_t::MODE_QAM16] = {16.0f, 28.0f, 0.0005f, 0.8f};
  d_mode_data_rates[modulation_mode_t::MODE_QAM16] = 9600;

  // 64-QAM @ 6,250 baud (6,250 × 6 = 37,500 bps)
  d_mode_thresholds[modulation_mode_t::MODE_QAM64_6250] = {20.0f, 32.0f, 0.0001f, 0.85f};
  d_mode_data_rates[modulation_mode_t::MODE_QAM64_6250] = 37500;

  // 64-QAM @ 12,500 baud (12,500 × 6 = 75,000 bps)
  d_mode_thresholds[modulation_mode_t::MODE_QAM64_12500] = {22.0f, 35.0f, 0.0001f, 0.9f};
  d_mode_data_rates[modulation_mode_t::MODE_QAM64_12500] = 75000;

  // 256-QAM @ 12,500 baud (12,500 × 8 = 100,000 bps)
  d_mode_thresholds[modulation_mode_t::MODE_QAM256] = {28.0f, 40.0f, 0.00005f, 0.95f};
  d_mode_data_rates[modulation_mode_t::MODE_QAM256] = 100000;

  // Tier 2: 12,500 baud PSK modes (constant envelope)
  // BPSK @ 12,500 baud (12,500 × 1 = 12,500 bps)
  d_mode_thresholds[modulation_mode_t::MODE_BPSK_12500] = {8.0f, 20.0f, 0.005f, 0.5f};
  d_mode_data_rates[modulation_mode_t::MODE_BPSK_12500] = 12500;

  // QPSK @ 12,500 baud (12,500 × 2 = 25,000 bps)
  d_mode_thresholds[modulation_mode_t::MODE_QPSK_12500] = {12.0f, 24.0f, 0.002f, 0.65f};
  d_mode_data_rates[modulation_mode_t::MODE_QPSK_12500] = 25000;

  // 8PSK @ 12,500 baud (12,500 × 3 = 37,500 bps)
  d_mode_thresholds[modulation_mode_t::MODE_8PSK_12500] = {16.0f, 28.0f, 0.0008f, 0.78f};
  d_mode_data_rates[modulation_mode_t::MODE_8PSK_12500] = 37500;

  // Tier 3: 12,500 baud QAM modes (variable envelope)
  // 16-QAM @ 12,500 baud (12,500 × 4 = 50,000 bps)
  d_mode_thresholds[modulation_mode_t::MODE_QAM16_12500] = {18.0f, 30.0f, 0.0003f, 0.82f};
  d_mode_data_rates[modulation_mode_t::MODE_QAM16_12500] = 50000;

  // Tier 4: Broadband SOQPSK modes (23cm/13cm bands, constant envelope)
  // SOQPSK @ 781 kbaud (1 Mbps) - ~1 MHz bandwidth
  d_mode_thresholds[modulation_mode_t::MODE_SOQPSK_1M] = {10.0f, 25.0f, 0.001f, 0.6f};
  d_mode_data_rates[modulation_mode_t::MODE_SOQPSK_1M] = 1000000;

  // SOQPSK @ 3.9 Mbaud (5 Mbps) - ~5 MHz bandwidth
  d_mode_thresholds[modulation_mode_t::MODE_SOQPSK_5M] = {15.0f, 30.0f, 0.0005f, 0.7f};
  d_mode_data_rates[modulation_mode_t::MODE_SOQPSK_5M] = 5000000;

  // SOQPSK @ 7.8 Mbaud (10 Mbps) - ~10 MHz bandwidth
  d_mode_thresholds[modulation_mode_t::MODE_SOQPSK_10M] = {18.0f, 33.0f, 0.0003f, 0.75f};
  d_mode_data_rates[modulation_mode_t::MODE_SOQPSK_10M] = 10000000;

  // SOQPSK @ 15.6 Mbaud (20 Mbps) - ~20 MHz bandwidth
  d_mode_thresholds[modulation_mode_t::MODE_SOQPSK_20M] = {22.0f, 36.0f, 0.0002f, 0.8f};
  d_mode_data_rates[modulation_mode_t::MODE_SOQPSK_20M] = 20000000;

  // SOQPSK @ 31.3 Mbaud (40 Mbps) - ~40 MHz bandwidth
  d_mode_thresholds[modulation_mode_t::MODE_SOQPSK_40M] = {26.0f, 40.0f, 0.0001f, 0.85f};
  d_mode_data_rates[modulation_mode_t::MODE_SOQPSK_40M] = 40000000;
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

bool adaptive_rate_control_impl::is_tier4_mode(modulation_mode_t mode) const {
  return (mode == modulation_mode_t::MODE_SOQPSK_1M ||
          mode == modulation_mode_t::MODE_SOQPSK_5M ||
          mode == modulation_mode_t::MODE_SOQPSK_10M ||
          mode == modulation_mode_t::MODE_SOQPSK_20M ||
          mode == modulation_mode_t::MODE_SOQPSK_40M);
}

void adaptive_rate_control_impl::set_modulation_mode(modulation_mode_t mode) {
  std::lock_guard<std::mutex> lock(d_mutex);
  // Reject Tier 4 modes if disabled
  if (is_tier4_mode(mode) && !d_enable_tier4) {
    // Silently fall back to current mode or default
    return;
  }
  d_current_mode = mode;
  d_last_mode = mode;
}

void adaptive_rate_control_impl::set_adaptation_enabled(bool enabled) {
  std::lock_guard<std::mutex> lock(d_mutex);
  d_adaptation_enabled = enabled;
}

void adaptive_rate_control_impl::set_tier4_enabled(bool enabled) {
  std::lock_guard<std::mutex> lock(d_mutex);
  d_enable_tier4 = enabled;
  // If Tier 4 is being disabled and current mode is Tier 4, fall back to default
  if (!d_enable_tier4 && is_tier4_mode(d_current_mode)) {
    d_current_mode = modulation_mode_t::MODE_2FSK;
    d_last_mode = modulation_mode_t::MODE_2FSK;
  }
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
      modulation_mode_t::MODE_SOQPSK_40M, modulation_mode_t::MODE_SOQPSK_20M,
      modulation_mode_t::MODE_SOQPSK_10M, modulation_mode_t::MODE_SOQPSK_5M,
      modulation_mode_t::MODE_SOQPSK_1M, modulation_mode_t::MODE_QAM256,
      modulation_mode_t::MODE_QAM64_12500, modulation_mode_t::MODE_QAM16_12500,
      modulation_mode_t::MODE_8PSK_12500, modulation_mode_t::MODE_QPSK_12500,
      modulation_mode_t::MODE_BPSK_12500, modulation_mode_t::MODE_QAM64_6250,
      modulation_mode_t::MODE_QAM16, modulation_mode_t::MODE_16FSK,
      modulation_mode_t::MODE_8PSK, modulation_mode_t::MODE_8FSK,
      modulation_mode_t::MODE_QPSK, modulation_mode_t::MODE_4FSK,
      modulation_mode_t::MODE_BPSK, modulation_mode_t::MODE_2FSK};

  for (auto mode : modes) {
    // Skip Tier 4 modes if disabled
    if (is_tier4_mode(mode) && !d_enable_tier4) {
      continue;
    }
    
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


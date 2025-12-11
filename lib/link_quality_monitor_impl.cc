/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "link_quality_monitor_impl.h"
#include <gnuradio/io_signature.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace gr {
namespace packet_protocols {

link_quality_monitor::sptr link_quality_monitor::make(float alpha, int update_period) {
  return gnuradio::make_block_sptr<link_quality_monitor_impl>(alpha, update_period);
}

link_quality_monitor_impl::link_quality_monitor_impl(float alpha, int update_period)
    : gr::sync_block("link_quality_monitor",
                     gr::io_signature::make(1, 1, sizeof(char)),
                     gr::io_signature::make(1, 1, sizeof(char))),
      d_alpha(alpha),
      d_update_period(update_period),
      d_sample_count(0),
      d_snr_db(0.0f),
      d_ber(0.0f),
      d_fer(0.0f),
      d_quality_score(0.5f),
      d_total_frames(0),
      d_error_frames(0),
      d_total_bits(0),
      d_error_bits(0) {
  // Clamp alpha to valid range
  d_alpha = std::max(0.0f, std::min(1.0f, alpha));
}

link_quality_monitor_impl::~link_quality_monitor_impl() {}

int link_quality_monitor_impl::work(int noutput_items,
                                    gr_vector_const_void_star& input_items,
                                    gr_vector_void_star& output_items) {
  const char* in = (const char*)input_items[0];
  char* out = (char*)output_items[0];

  // Pass through data
  memcpy(out, in, noutput_items * sizeof(char));

  d_sample_count += noutput_items;

  // Update quality score periodically
  if (d_sample_count >= d_update_period) {
    std::lock_guard<std::mutex> lock(d_mutex);

    // Update FER
    if (d_total_frames > 0) {
      d_fer = static_cast<float>(d_error_frames) / static_cast<float>(d_total_frames);
    }

    // Update BER
    if (d_total_bits > 0) {
      d_ber = static_cast<float>(d_error_bits) / static_cast<float>(d_total_bits);
    }

    // Calculate quality score
    d_quality_score = calculate_quality_score(d_snr_db, d_ber, d_fer);

    d_sample_count = 0;
  }

  return noutput_items;
}

float link_quality_monitor_impl::get_snr() const {
  std::lock_guard<std::mutex> lock(d_mutex);
  return d_snr_db;
}

float link_quality_monitor_impl::get_ber() const {
  std::lock_guard<std::mutex> lock(d_mutex);
  return d_ber;
}

float link_quality_monitor_impl::get_fer() const {
  std::lock_guard<std::mutex> lock(d_mutex);
  return d_fer;
}

float link_quality_monitor_impl::get_quality_score() const {
  std::lock_guard<std::mutex> lock(d_mutex);
  return d_quality_score;
}

void link_quality_monitor_impl::update_snr(float snr_db) {
  std::lock_guard<std::mutex> lock(d_mutex);

  // Exponential moving average
  if (d_snr_history.empty()) {
    d_snr_db = snr_db;
  } else {
    d_snr_db = d_alpha * snr_db + (1.0f - d_alpha) * d_snr_db;
  }

  // Maintain history
  d_snr_history.push_back(snr_db);
  if (d_snr_history.size() > MAX_HISTORY) {
    d_snr_history.pop_front();
  }
}

void link_quality_monitor_impl::update_ber(float ber) {
  std::lock_guard<std::mutex> lock(d_mutex);

  // Clamp BER to valid range
  ber = std::max(0.0f, std::min(1.0f, ber));

  // Exponential moving average
  if (d_ber_history.empty()) {
    d_ber = ber;
  } else {
    d_ber = d_alpha * ber + (1.0f - d_alpha) * d_ber;
  }

  // Maintain history
  d_ber_history.push_back(ber);
  if (d_ber_history.size() > MAX_HISTORY) {
    d_ber_history.pop_front();
  }
}

void link_quality_monitor_impl::record_frame_error() {
  std::lock_guard<std::mutex> lock(d_mutex);
  d_total_frames++;
  d_error_frames++;
  // Update FER immediately
  if (d_total_frames > 0) {
    d_fer = static_cast<float>(d_error_frames) / static_cast<float>(d_total_frames);
  }
}

void link_quality_monitor_impl::record_frame_success() {
  std::lock_guard<std::mutex> lock(d_mutex);
  d_total_frames++;
  // Update FER immediately
  if (d_total_frames > 0) {
    d_fer = static_cast<float>(d_error_frames) / static_cast<float>(d_total_frames);
  }
}

void link_quality_monitor_impl::reset() {
  std::lock_guard<std::mutex> lock(d_mutex);
  d_snr_db = 0.0f;
  d_ber = 0.0f;
  d_fer = 0.0f;
  d_quality_score = 0.5f;
  d_total_frames = 0;
  d_error_frames = 0;
  d_total_bits = 0;
  d_error_bits = 0;
  d_snr_history.clear();
  d_ber_history.clear();
}

float link_quality_monitor_impl::calculate_quality_score(float snr_db, float ber,
                                                           float fer) const {
  // Normalize SNR (assume good link is > 10 dB, excellent is > 20 dB)
  float snr_score = std::max(0.0f, std::min(1.0f, (snr_db + 10.0f) / 30.0f));

  // BER score (lower is better, assume good link has BER < 1e-3)
  float ber_score = std::max(0.0f, std::min(1.0f, 1.0f - (ber * 1000.0f)));

  // FER score (lower is better, assume good link has FER < 0.1)
  float fer_score = std::max(0.0f, std::min(1.0f, 1.0f - (fer * 10.0f)));

  // Weighted combination (SNR is most important)
  return 0.5f * snr_score + 0.3f * ber_score + 0.2f * fer_score;
}

} // namespace packet_protocols
} // namespace gr


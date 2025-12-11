/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 *
 * gr-packet-protocols is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 */

#ifndef INCLUDED_PACKET_PROTOCOLS_LINK_QUALITY_MONITOR_IMPL_H
#define INCLUDED_PACKET_PROTOCOLS_LINK_QUALITY_MONITOR_IMPL_H

#include <gnuradio/packet_protocols/link_quality_monitor.h>
#include <deque>
#include <mutex>

namespace gr {
namespace packet_protocols {

class link_quality_monitor_impl : public link_quality_monitor {
 private:
  float d_alpha;              // Smoothing factor
  int d_update_period;        // Samples between updates
  int d_sample_count;         // Sample counter

  // Quality metrics (with mutex protection)
  mutable std::mutex d_mutex;
  float d_snr_db;            // Current SNR estimate (dB)
  float d_ber;               // Current BER estimate
  float d_fer;               // Frame error rate
  float d_quality_score;     // Composite quality score (0.0-1.0)

  // Statistics
  uint64_t d_total_frames;
  uint64_t d_error_frames;
  uint64_t d_total_bits;
  uint64_t d_error_bits;

  // Moving averages
  std::deque<float> d_snr_history;
  std::deque<float> d_ber_history;
  static constexpr size_t MAX_HISTORY = 100;

  // Calculate quality score from metrics
  float calculate_quality_score(float snr_db, float ber, float fer) const;

 public:
  link_quality_monitor_impl(float alpha, int update_period);
  ~link_quality_monitor_impl();

  int work(int noutput_items, gr_vector_const_void_star& input_items,
           gr_vector_void_star& output_items) override;

  float get_snr() const override;
  float get_ber() const override;
  float get_fer() const override;
  float get_quality_score() const override;
  void update_snr(float snr_db) override;
  void update_ber(float ber) override;
  void record_frame_error() override;
  void record_frame_success() override;
  void reset() override;
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_LINK_QUALITY_MONITOR_IMPL_H */


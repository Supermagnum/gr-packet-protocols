/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 */

#ifndef INCLUDED_PACKET_PROTOCOLS_ADAPTIVE_RATE_CONTROL_IMPL_H
#define INCLUDED_PACKET_PROTOCOLS_ADAPTIVE_RATE_CONTROL_IMPL_H

#include <gnuradio/packet_protocols/adaptive_rate_control.h>
#include <map>
#include <mutex>

namespace gr {
namespace packet_protocols {

class adaptive_rate_control_impl : public adaptive_rate_control {
 private:
  modulation_mode_t d_current_mode;
  bool d_adaptation_enabled;
  float d_hysteresis_db;
  float d_last_snr_db;
  modulation_mode_t d_last_mode;

  // Mode thresholds
  std::map<modulation_mode_t, rate_thresholds_t> d_mode_thresholds;

  // Data rates for each mode (bits per second)
  std::map<modulation_mode_t, int> d_mode_data_rates;

  mutable std::mutex d_mutex;

  // Initialize mode thresholds
  void initialize_thresholds();

  // Get threshold for a mode
  rate_thresholds_t get_thresholds(modulation_mode_t mode) const;

 public:
  adaptive_rate_control_impl(modulation_mode_t initial_mode, bool enable_adaptation,
                              float hysteresis_db);
  ~adaptive_rate_control_impl();

  int work(int noutput_items, gr_vector_const_void_star& input_items,
           gr_vector_void_star& output_items) override;

  modulation_mode_t get_modulation_mode() const override;
  void set_modulation_mode(modulation_mode_t mode) override;
  void set_adaptation_enabled(bool enabled) override;
  void update_quality(float snr_db, float ber, float quality_score) override;
  modulation_mode_t recommend_mode(float snr_db, float ber) const override;
  int get_data_rate() const override;
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_ADAPTIVE_RATE_CONTROL_IMPL_H */


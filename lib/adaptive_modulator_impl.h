/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 */

#ifndef INCLUDED_PACKET_PROTOCOLS_ADAPTIVE_MODULATOR_IMPL_H
#define INCLUDED_PACKET_PROTOCOLS_ADAPTIVE_MODULATOR_IMPL_H

#include <gnuradio/packet_protocols/adaptive_modulator.h>
#include <gnuradio/packet_protocols/adaptive_rate_control.h>
#include <gnuradio/blocks/selector.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/digital/gfsk_mod.h>
#include <gnuradio/digital/psk_mod.h>
#include <gnuradio/digital/qam_mod.h>
#include <map>
#include <mutex>

namespace gr {
namespace packet_protocols {

class adaptive_modulator_impl : public adaptive_modulator {
 private:
  adaptive_rate_control::sptr d_rate_control;
  blocks::selector::sptr d_selector;
  int d_samples_per_symbol;
  modulation_mode_t d_current_mode;
  modulation_mode_t d_last_mode;

  // Modulation blocks for each mode
  std::map<modulation_mode_t, gr::basic_block_sptr> d_mod_blocks;
  std::map<modulation_mode_t, int> d_mode_to_index;

  mutable std::mutex d_mutex;

  // Create modulation block for a specific mode
  gr::basic_block_sptr create_modulator(modulation_mode_t mode);

  // Update selector to use new mode
  void switch_modulation_mode(modulation_mode_t new_mode);

  // Monitor mode changes
  void monitor_mode_changes();

 public:
  adaptive_modulator_impl(modulation_mode_t initial_mode, int samples_per_symbol,
                          bool enable_adaptation, float hysteresis_db);
  ~adaptive_modulator_impl();

  adaptive_rate_control::sptr get_rate_control() override;
  modulation_mode_t get_modulation_mode() const override;
  void set_modulation_mode(modulation_mode_t mode) override;
  void set_adaptation_enabled(bool enabled) override;
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_ADAPTIVE_MODULATOR_IMPL_H */


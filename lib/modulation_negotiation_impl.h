/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 */

#ifndef INCLUDED_PACKET_PROTOCOLS_MODULATION_NEGOTIATION_IMPL_H
#define INCLUDED_PACKET_PROTOCOLS_MODULATION_NEGOTIATION_IMPL_H

#include <gnuradio/packet_protocols/modulation_negotiation.h>
#include <map>
#include <mutex>
#include <string>

namespace gr {
namespace packet_protocols {

class modulation_negotiation_impl : public modulation_negotiation {
 private:
  std::string d_station_id;
  std::vector<modulation_mode_t> d_supported_modes;
  int d_negotiation_timeout_ms;

  // Negotiation state
  mutable std::mutex d_mutex;
  bool d_negotiating;
  std::string d_remote_station_id;
  modulation_mode_t d_negotiated_mode;
  modulation_mode_t d_pending_mode;
  uint64_t d_negotiation_start_time;

  // Active negotiations per station
  std::map<std::string, modulation_mode_t> d_negotiated_modes;

 public:
  modulation_negotiation_impl(const std::string& station_id,
                               const std::vector<modulation_mode_t>& supported_modes,
                               int negotiation_timeout_ms);
  ~modulation_negotiation_impl();

  int work(int noutput_items, gr_vector_const_void_star& input_items,
           gr_vector_void_star& output_items) override;

  void initiate_negotiation(const std::string& remote_station_id,
                             modulation_mode_t proposed_mode) override;
  modulation_mode_t get_negotiated_mode() const override;
  bool is_negotiating() const override;
  void send_quality_feedback(const std::string& remote_station_id, float snr_db,
                               float ber, float quality_score) override;
  std::vector<modulation_mode_t> get_supported_modes() const override;
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_MODULATION_NEGOTIATION_IMPL_H */


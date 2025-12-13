/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 *
 * gr-packet-protocols is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * gr-packet-protocols is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gr-packet-protocols; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_PACKET_PROTOCOLS_MODULATION_NEGOTIATION_H
#define INCLUDED_PACKET_PROTOCOLS_MODULATION_NEGOTIATION_H

#include <gnuradio/packet_protocols/api.h>
#include <gnuradio/sync_block.h>
#include <gnuradio/packet_protocols/adaptive_rate_control.h>
#include <string>
#include <vector>

namespace gr {
namespace packet_protocols {

// Negotiation message types
enum class negotiation_msg_type_t {
  NEG_REQ = 0,      // Negotiation request
  NEG_RESP = 1,     // Negotiation response
  NEG_ACK = 2,      // Negotiation acknowledgment
  NEG_REJECT = 3,   // Negotiation rejection
  MODE_CHANGE = 4,  // Mode change notification
  QUALITY_FEEDBACK = 5  // Link quality feedback
};

// Negotiation message structure
struct negotiation_msg_t {
  negotiation_msg_type_t type;
  modulation_mode_t proposed_mode;
  modulation_mode_t supported_modes[8];  // List of supported modes
  int num_supported_modes;
  float snr_db;                          // Current SNR
  float ber;                             // Current BER
  float quality_score;                   // Link quality score
  std::string station_id;                // Station identifier
};

/*!
 * \brief Modulation Negotiation Handler
 * \ingroup packet_protocols
 *
 * Handles negotiation of modulation modes and rates between stations.
 * Extends KISS protocol with negotiation capabilities.
 */
class PACKET_PROTOCOLS_API modulation_negotiation : virtual public gr::sync_block {
 public:
  typedef std::shared_ptr<modulation_negotiation> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of packet_protocols::modulation_negotiation.
   *
   * \param station_id Station identifier (callsign)
   * \param supported_modes List of supported modulation modes
   * \param negotiation_timeout_ms Timeout for negotiation in milliseconds
   */
  static sptr make(const std::string& station_id,
                   const std::vector<modulation_mode_t>& supported_modes,
                   int negotiation_timeout_ms = 5000);

  /*!
   * \brief Initiate negotiation with remote station
   * \param remote_station_id Remote station identifier
   * \param proposed_mode Proposed modulation mode
   */
  virtual void initiate_negotiation(const std::string& remote_station_id,
                                     modulation_mode_t proposed_mode) = 0;

  /*!
   * \brief Get current negotiated mode
   */
  virtual modulation_mode_t get_negotiated_mode() const = 0;

  /*!
   * \brief Check if negotiation is in progress
   */
  virtual bool is_negotiating() const = 0;

  /*!
   * \brief Send quality feedback to remote station
   * \param remote_station_id Remote station identifier
   * \param snr_db Current SNR
   * \param ber Current BER
   * \param quality_score Link quality score
   */
  virtual void send_quality_feedback(const std::string& remote_station_id,
                                      float snr_db, float ber, float quality_score) = 0;

  /*!
   * \brief Get list of supported modes
   */
  virtual std::vector<modulation_mode_t> get_supported_modes() const = 0;

  /*!
   * \brief Set callback for sending KISS frames
   * \param callback Function to call when sending KISS frames
   */
  virtual void set_kiss_frame_sender(
      std::function<void(uint8_t, const std::vector<uint8_t>&)> callback) = 0;

  /*!
   * \brief Enable automatic negotiation when mode changes
   * \param enabled Enable automatic negotiation
   * \param rate_control Pointer to adaptive_rate_control to monitor for mode changes
   */
  virtual void set_auto_negotiation_enabled(bool enabled,
                                             adaptive_rate_control* rate_control = nullptr) = 0;
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_MODULATION_NEGOTIATION_H */


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

#ifndef INCLUDED_PACKET_PROTOCOLS_ADAPTIVE_RATE_CONTROL_H
#define INCLUDED_PACKET_PROTOCOLS_ADAPTIVE_RATE_CONTROL_H

#include <gnuradio/packet_protocols/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace packet_protocols {

// Modulation modes
enum class modulation_mode_t {
  MODE_2FSK = 0,   // Binary FSK (Bell 202 / AX.25, 1200 baud)
  MODE_4FSK = 1,   // 4-level FSK
  MODE_8FSK = 2,   // 8-level FSK
  MODE_16FSK = 3,  // 16-level FSK
  MODE_BPSK = 4,   // Binary PSK
  MODE_QPSK = 5,   // Quadrature PSK
  MODE_8PSK = 6,   // 8-PSK
  MODE_QAM16 = 7,  // 16-QAM @ 2,400 baud (2,400 × 4 = 9,600 bps)
  MODE_QAM64_6250 = 8,  // 64-QAM @ 6,250 baud (6,250 × 6 = 37,500 bps)
  MODE_QAM64_12500 = 9,  // 64-QAM @ 12,500 baud (12,500 × 6 = 75,000 bps)
  MODE_QAM256 = 10,  // 256-QAM @ 12,500 baud (12,500 × 8 = 100,000 bps)
  // Tier 2: 12,500 baud PSK modes (constant envelope)
  MODE_BPSK_12500 = 11,  // BPSK @ 12,500 baud (12,500 × 1 = 12,500 bps)
  MODE_QPSK_12500 = 12,  // QPSK @ 12,500 baud (12,500 × 2 = 25,000 bps)
  MODE_8PSK_12500 = 13,  // 8PSK @ 12,500 baud (12,500 × 3 = 37,500 bps)
  // Tier 3: 12,500 baud QAM modes (variable envelope)
  MODE_QAM16_12500 = 14,  // 16-QAM @ 12,500 baud (12,500 × 4 = 50,000 bps)
  // Tier 4: Broadband SOQPSK modes (23cm/13cm bands, constant envelope)
  MODE_SOQPSK_1M = 15,    // SOQPSK @ 781 kbaud (1 Mbps) - ~1 MHz bandwidth
  MODE_SOQPSK_5M = 16,    // SOQPSK @ 3.9 Mbaud (5 Mbps) - ~5 MHz bandwidth
  MODE_SOQPSK_10M = 17,   // SOQPSK @ 7.8 Mbaud (10 Mbps) - ~10 MHz bandwidth
  MODE_SOQPSK_20M = 18,   // SOQPSK @ 15.6 Mbaud (20 Mbps) - ~20 MHz bandwidth
  MODE_SOQPSK_40M = 19    // SOQPSK @ 31.3 Mbaud (40 Mbps) - ~40 MHz bandwidth
};

// Rate adaptation thresholds
struct rate_thresholds_t {
  float snr_min_dB;      // Minimum SNR for this mode (dB)
  float snr_max_dB;      // Maximum SNR for this mode (dB)
  float ber_max;         // Maximum acceptable BER
  float quality_min;     // Minimum quality score
};

/*!
 * \brief Adaptive Rate Control
 * \ingroup packet_protocols
 *
 * Automatically adjusts modulation mode and data rate based on link quality.
 */
class PACKET_PROTOCOLS_API adaptive_rate_control : virtual public gr::sync_block {
 public:
  typedef std::shared_ptr<adaptive_rate_control> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of packet_protocols::adaptive_rate_control.
   *
   * \param initial_mode Initial modulation mode
   * \param enable_adaptation Enable automatic rate adaptation
   * \param hysteresis_db Hysteresis in dB to prevent rapid switching
   * \param enable_tier4 Enable Tier 4 broadband modes (1-40 Mbps, disabled by default to prevent out-of-bandwidth transmissions)
   */
  static sptr make(modulation_mode_t initial_mode = modulation_mode_t::MODE_2FSK,
                   bool enable_adaptation = true, float hysteresis_db = 2.0,
                   bool enable_tier4 = false);

  /*!
   * \brief Get current modulation mode
   */
  virtual modulation_mode_t get_modulation_mode() const = 0;

  /*!
   * \brief Set modulation mode manually
   * \param mode Modulation mode
   */
  virtual void set_modulation_mode(modulation_mode_t mode) = 0;

  /*!
   * \brief Enable/disable automatic adaptation
   * \param enabled Enable adaptation
   */
  virtual void set_adaptation_enabled(bool enabled) = 0;

  /*!
   * \brief Enable/disable Tier 4 broadband modes (1-40 Mbps)
   * \param enabled Enable Tier 4 modes (disabled by default to prevent out-of-bandwidth transmissions)
   * 
   * WARNING: Tier 4 modes require 1-40 MHz bandwidth and are designed for 23cm/13cm bands only.
   * Enabling Tier 4 on standard 12.5 kHz VHF/UHF channels will cause out-of-bandwidth transmissions.
   */
  virtual void set_tier4_enabled(bool enabled) = 0;

  /*!
   * \brief Update link quality metrics and adapt if enabled
   * \param snr_db Current SNR in dB
   * \param ber Current BER
   * \param quality_score Link quality score (0.0-1.0)
   */
  virtual void update_quality(float snr_db, float ber, float quality_score) = 0;

  /*!
   * \brief Get recommended modulation mode based on quality
   * \param snr_db Current SNR in dB
   * \param ber Current BER
   * \return Recommended modulation mode
   */
  virtual modulation_mode_t recommend_mode(float snr_db, float ber) const = 0;

  /*!
   * \brief Get data rate for current mode (bits per second)
   */
  virtual int get_data_rate() const = 0;
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_ADAPTIVE_RATE_CONTROL_H */


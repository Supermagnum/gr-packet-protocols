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
  MODE_2FSK = 0,   // Binary FSK
  MODE_4FSK = 1,   // 4-level FSK
  MODE_8FSK = 2,   // 8-level FSK
  MODE_16FSK = 3,  // 16-level FSK
  MODE_BPSK = 4,   // Binary PSK
  MODE_QPSK = 5,   // Quadrature PSK
  MODE_8PSK = 6,   // 8-PSK
  MODE_QAM16 = 7,  // 16-QAM
  MODE_QAM64 = 8   // 64-QAM
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
   */
  static sptr make(modulation_mode_t initial_mode = modulation_mode_t::MODE_4FSK,
                   bool enable_adaptation = true, float hysteresis_db = 2.0);

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


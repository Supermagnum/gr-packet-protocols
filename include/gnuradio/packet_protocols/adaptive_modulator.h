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

#ifndef INCLUDED_PACKET_PROTOCOLS_ADAPTIVE_MODULATOR_H
#define INCLUDED_PACKET_PROTOCOLS_ADAPTIVE_MODULATOR_H

#include <gnuradio/packet_protocols/api.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/packet_protocols/adaptive_rate_control.h>

namespace gr {
namespace packet_protocols {

/*!
 * \brief Adaptive Modulator
 * \ingroup packet_protocols
 *
 * Hierarchical block that automatically switches between different modulation
 * modes based on link quality. Integrates adaptive_rate_control with actual
 * GNU Radio modulation blocks.
 *
 * This block creates and manages multiple modulation blocks internally and
 * switches between them based on the adaptive rate control decisions.
 */
class PACKET_PROTOCOLS_API adaptive_modulator : virtual public gr::hier_block2 {
 public:
  typedef std::shared_ptr<adaptive_modulator> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of packet_protocols::adaptive_modulator.
   *
   * \param initial_mode Initial modulation mode
   * \param samples_per_symbol Samples per symbol for modulation
   * \param enable_adaptation Enable automatic rate adaptation
   * \param hysteresis_db Hysteresis in dB to prevent rapid switching
   */
  static sptr make(modulation_mode_t initial_mode = modulation_mode_t::MODE_4FSK,
                   int samples_per_symbol = 2, bool enable_adaptation = true,
                   float hysteresis_db = 2.0);

  /*!
   * \brief Get the adaptive rate control block
   * \return Shared pointer to adaptive_rate_control block
   */
  virtual adaptive_rate_control::sptr get_rate_control() = 0;

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
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_ADAPTIVE_MODULATOR_H */


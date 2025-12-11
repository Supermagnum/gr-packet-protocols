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

#ifndef INCLUDED_PACKET_PROTOCOLS_LINK_QUALITY_MONITOR_H
#define INCLUDED_PACKET_PROTOCOLS_LINK_QUALITY_MONITOR_H

#include <gnuradio/packet_protocols/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace packet_protocols {

/*!
 * \brief Link Quality Monitor
 * \ingroup packet_protocols
 *
 * Monitors link quality metrics including SNR, BER, and frame error rate.
 * Provides feedback for adaptive rate control.
 */
class PACKET_PROTOCOLS_API link_quality_monitor : virtual public gr::sync_block {
 public:
  typedef std::shared_ptr<link_quality_monitor> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of packet_protocols::link_quality_monitor.
   *
   * \param alpha Smoothing factor for moving average (0.0-1.0)
   * \param update_period Number of samples between quality updates
   */
  static sptr make(float alpha = 0.1, int update_period = 1000);

  /*!
   * \brief Get current SNR estimate (dB)
   */
  virtual float get_snr() const = 0;

  /*!
   * \brief Get current BER estimate
   */
  virtual float get_ber() const = 0;

  /*!
   * \brief Get frame error rate
   */
  virtual float get_fer() const = 0;

  /*!
   * \brief Get link quality score (0.0-1.0, higher is better)
   */
  virtual float get_quality_score() const = 0;

  /*!
   * \brief Update SNR measurement
   * \param snr_db SNR in dB
   */
  virtual void update_snr(float snr_db) = 0;

  /*!
   * \brief Update BER measurement
   * \param ber Bit error rate (0.0-1.0)
   */
  virtual void update_ber(float ber) = 0;

  /*!
   * \brief Record frame error
   */
  virtual void record_frame_error() = 0;

  /*!
   * \brief Record successful frame
   */
  virtual void record_frame_success() = 0;

  /*!
   * \brief Reset statistics
   */
  virtual void reset() = 0;
};

} // namespace packet_protocols
} // namespace gr

#endif /* INCLUDED_PACKET_PROTOCOLS_LINK_QUALITY_MONITOR_H */


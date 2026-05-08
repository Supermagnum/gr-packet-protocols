// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_LINK_QUALITY_MONITOR_HPP
#define GNURADIO4_PACKET_PROTOCOLS_LINK_QUALITY_MONITOR_HPP

#include <gnuradio-4.0/packet_protocols/detail/Helpers.hpp>

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/Value.hpp>
#include <gnuradio-4.0/annotated.hpp>

#include <algorithm>
#include <cmath>
#include <span>
#include <string_view>
#include <vector>

namespace gnuradio4::packet_protocols {

GR_REGISTER_BLOCK(gnuradio4::packet_protocols::LinkQualityMonitor)

struct LinkQualityMonitor : gr::Block<LinkQualityMonitor, gr::NoTagPropagation> {
    using Description =
        gr::Doc<"Publishes link-quality NOTIFY messages using SNR/BER/FER from block settings unless message fields override them.">;

    gr::MsgPortIn  observe_in{};
    gr::MsgPortOut quality_metric_out{};

    gr::Annotated<float, "snr_db_measurement"> snr_db_measurement{-5.F};
    gr::Annotated<float, "ber_measurement"> ber_measurement{0.F};
    gr::Annotated<float, "fer_measurement"> fer_measurement{0.F};

    gr::Annotated<float, "snr_threshold_db"> snr_threshold_db{10.F};
    gr::Annotated<float, "ber_threshold"> ber_threshold{0.01F};
    gr::Annotated<float, "fer_threshold"> fer_threshold{0.05F};

    GR_MAKE_REFLECTABLE(LinkQualityMonitor, observe_in, quality_metric_out, snr_db_measurement, ber_measurement, fer_measurement,
                        snr_threshold_db, ber_threshold, fer_threshold);

    void start() noexcept {}

    [[nodiscard]] gr::work::Status processBulk() noexcept { return gr::work::Status::OK; }

    static float quality_score_calc(float snr_db, float ber, float fer) noexcept {
        const float snr_score = std::max(0.F, std::min(1.F, (snr_db + 10.F) / 30.F));
        const float ber_score = std::max(0.F, std::min(1.F, 1.F - (ber * 1000.F)));
        const float fer_score = std::max(0.F, std::min(1.F, 1.F - (fer * 10.F)));
        return 0.5F * snr_score + 0.3F * ber_score + 0.2F * fer_score;
    }

    static float merge_float(const gr::property_map& body, std::string_view field, float fallback) noexcept {
        if (auto v = detail::float_from_map(body, field)) {
            return *v;
        }
        return fallback;
    }

    void processMessages(gr::MsgPortIn& port, std::span<const gr::Message> messages) {
        if (std::addressof(port) != std::addressof(observe_in)) [[unlikely]] {
            return;
        }
        if (messages.empty()) [[unlikely]] {
            return;
        }
        for (const gr::Message& msg : messages) {
            if (!msg.data.has_value()) [[unlikely]] {
                continue;
            }
            const gr::property_map& body = msg.data.value();
            float                    s   = merge_float(body, std::string_view{"snr_db"}, snr_db_measurement.value);
            float b = merge_float(body, std::string_view{"ber"}, ber_measurement.value);
            float f = merge_float(body, std::string_view{"fer"}, fer_measurement.value);
            b             = std::max(0.F, std::min(1.F, b));
            f             = std::max(0.F, std::min(1.F, f));
            const bool pass = (s >= snr_threshold_db.value) && (b <= ber_threshold.value) && (f <= fer_threshold.value);
            const float q = quality_score_calc(s, b, f);

            gr::property_map out;
            out[gr::convert_string_domain(std::string_view("snr_db"))]  = gr::pmt::Value(s);
            out[gr::convert_string_domain(std::string_view("ber"))]      = gr::pmt::Value(b);
            out[gr::convert_string_domain(std::string_view("fer"))]       = gr::pmt::Value(f);
            out[gr::convert_string_domain(std::string_view("quality"))] = gr::pmt::Value(q);
            std::vector<std::uint8_t> pass_tensor{static_cast<std::uint8_t>(pass ? 1U : 0U)};
            out[gr::convert_string_domain(std::string_view("passed_thresholds"))] =
                gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(pass_tensor)));
            gr::Message routed{};
            routed.cmd = gr::message::Command::Notify;
            routed.data = std::move(out);
            auto ws     = quality_metric_out.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
            ws[0]       = std::move(routed);
            ws.publish(1UZ);
        }
    }
};

} // namespace gnuradio4::packet_protocols

#endif

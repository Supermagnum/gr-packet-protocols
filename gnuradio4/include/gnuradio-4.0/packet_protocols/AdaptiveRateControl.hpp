// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_ADAPTIVE_RATE_CONTROL_HPP
#define GNURADIO4_PACKET_PROTOCOLS_ADAPTIVE_RATE_CONTROL_HPP

#include <gnuradio-4.0/packet_protocols/detail/AdaptiveRateEngine.hpp>
#include <gnuradio-4.0/packet_protocols/detail/Helpers.hpp>

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/Value.hpp>
#include <gnuradio-4.0/annotated.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace gnuradio4::packet_protocols {

GR_REGISTER_BLOCK(gnuradio4::packet_protocols::AdaptiveRateControl)

struct AdaptiveRateControl : gr::Block<AdaptiveRateControl, gr::NoTagPropagation> {
    using Description = gr::Doc<"Translates NOTIFY quality probes into modulation changes using the ported GR3 heuristic tables (message-only deterministic block).">;

    gr::MsgPortIn  quality_metric_in{};
    gr::MsgPortOut mode_change_out{};

    gr::Annotated<std::uint8_t, "adaptation_seed_mode"> adaptation_seed_mode{
        static_cast<std::uint8_t>(detail::ModulationMode::mode_qam16)};
    gr::Annotated<bool, "adaptation_enabled"> adaptation_enabled{
        true};
    gr::Annotated<float, "hysteresis_db"> hysteresis_db{2.F};
    gr::Annotated<bool, "tier4_enabled"> tier4_enabled{false};

    GR_MAKE_REFLECTABLE(AdaptiveRateControl, quality_metric_in, mode_change_out, adaptation_seed_mode, adaptation_enabled, hysteresis_db,
                         tier4_enabled);

    std::optional<detail::AdaptiveRateEngine> engine{};

    void start() noexcept {
        engine.emplace(static_cast<detail::ModulationMode>(adaptation_seed_mode.value),
                       adaptation_enabled.value,
                       hysteresis_db.value,
                       tier4_enabled.value);
    }

    [[nodiscard]] gr::work::Status processBulk() noexcept { return gr::work::Status::OK; }

    void processMessages(gr::MsgPortIn& port, std::span<const gr::Message> messages) {
        if (std::addressof(port) != std::addressof(quality_metric_in)) [[unlikely]] {
            return;
        }
        if (!engine.has_value()) [[unlikely]] {
            start();
        }
        if (!engine.has_value()) {
            return;
        }
        for (const gr::Message& msg : messages) {
            if (!msg.data.has_value()) {
                continue;
            }
            const gr::property_map& m = msg.data.value();
            const float             snr = detail::float_from_map(m, std::string_view{"snr_db"}).value_or(0.F);
            const float             ber =
                detail::float_from_map(m, std::string_view{"ber"}).value_or(0.F);
            const float q =
                detail::float_from_map(m, std::string_view{"quality"}).value_or(0.F);
            const detail::ModulationMode before_snapshot = engine->current_mode();
            const bool switched                          = engine->update_quality(snr, ber, q);
            if (!switched) {
                continue;
            }
            const detail::ModulationMode now_mode = engine->current_mode();

            auto tensor_mode = [&](detail::ModulationMode mode_v) -> gr::pmt::Value {
                std::vector<std::uint8_t> blob{static_cast<std::uint8_t>(mode_v)};
                return gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(blob)));
            };

            gr::property_map outgoing;
            outgoing[gr::convert_string_domain(std::string_view{"previous_modulation_mode"})] = tensor_mode(before_snapshot);
            outgoing[gr::convert_string_domain(std::string_view{"current_modulation_mode"})] =
                tensor_mode(now_mode);

            gr::Message push{};
            push.cmd  = gr::message::Command::Notify;
            push.data = std::move(outgoing);
            auto wr   = mode_change_out.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
            wr[0]     = std::move(push);
            wr.publish(1UZ);
        }
    }
};

} // namespace gnuradio4::packet_protocols

#endif

// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_ADAPTIVE_MODULATOR_HPP
#define GNURADIO4_PACKET_PROTOCOLS_ADAPTIVE_MODULATOR_HPP

#include <gnuradio-4.0/packet_protocols/detail/AdaptiveRateEngine.hpp>
#include <gnuradio-4.0/packet_protocols/detail/Helpers.hpp>
#include <gnuradio-4.0/packet_protocols/detail/KissFraming.hpp>
#include <gnuradio-4.0/packet_protocols/detail/ModeList.hpp>
#include <gnuradio-4.0/packet_protocols/detail/NegotiationService.hpp>

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/Value.hpp>
#include <gnuradio-4.0/annotated.hpp>

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gnuradio4::packet_protocols {

GR_REGISTER_BLOCK(gnuradio4::packet_protocols::AdaptiveModulator)

struct AdaptiveModulator : gr::Block<AdaptiveModulator, gr::NoTagPropagation> {
    using Description = gr::Doc<"Chains adaptive rate selection with negotiation handling; publishes KISS-framed negotiation traffic on kiss_transport_out plus mode tensors on mode_change_out.">;

    gr::MsgPortIn  quality_metric_in{};
    gr::MsgPortIn  negotiation_pdu_in{};
    gr::MsgPortOut kiss_transport_out{};
    gr::MsgPortOut mode_change_out{};

    gr::Annotated<std::string, "station_identifier">   station_identifier{std::string("GROUND")};
    gr::Annotated<std::string, "supported_modes_csv"> supported_modes_csv{std::string("0,1,5,7")};

    gr::Annotated<std::uint8_t, "adaptation_seed_mode"> adaptation_seed_mode{
        static_cast<std::uint8_t>(detail::ModulationMode::mode_qam16)};
    gr::Annotated<bool, "adaptation_enabled"> adaptation_enabled{true};
    gr::Annotated<float, "hysteresis_db"> hysteresis_db{2.F};
    gr::Annotated<bool, "tier4_enabled"> tier4_enabled{false};

    GR_MAKE_REFLECTABLE(AdaptiveModulator, quality_metric_in, negotiation_pdu_in, kiss_transport_out, mode_change_out, station_identifier,
                         supported_modes_csv, adaptation_seed_mode, adaptation_enabled, hysteresis_db, tier4_enabled);

    std::optional<detail::AdaptiveRateEngine> rate_engine{};
    detail::ModulationMode                   negotiated_mode{detail::ModulationMode::mode_4fsk};
    std::unordered_map<std::string, detail::ModulationMode> negotiated_peers{};

    void start() noexcept {
        rate_engine.emplace(static_cast<detail::ModulationMode>(adaptation_seed_mode.value),
                            adaptation_enabled.value,
                            hysteresis_db.value,
                            tier4_enabled.value);
    }

    [[nodiscard]] gr::work::Status processBulk() noexcept { return gr::work::Status::OK; }

    void publish_mode_change_tensors(detail::ModulationMode previous, detail::ModulationMode current) {
        auto tensor_mode = [](detail::ModulationMode m) -> gr::pmt::Value {
            std::vector<std::uint8_t> b{static_cast<std::uint8_t>(m)};
            return gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(b)));
        };
        gr::property_map pb;
        pb[gr::convert_string_domain(std::string_view{"previous_modulation_mode"})] = tensor_mode(previous);
        pb[gr::convert_string_domain(std::string_view{"current_modulation_mode"})] =
            tensor_mode(current);
        gr::Message m{};
        m.cmd  = gr::message::Command::Notify;
        m.data = std::move(pb);
        auto wr = mode_change_out.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
        wr[0]   = std::move(m);
        wr.publish(1UZ);
    }

    void publish_kiss_wire(std::uint8_t kiss_cmd, std::vector<std::uint8_t>&& inner) {
        std::vector<std::uint8_t> wire = detail::kissFrame(kiss_cmd, std::move(inner));
        gr::property_map          pb;
        pb[gr::convert_string_domain(std::string_view{"kiss_frame_bytes"})] =
            gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(wire)));
        gr::Message routed{};
        routed.cmd  = gr::message::Command::Notify;
        routed.data = std::move(pb);
        auto wr = kiss_transport_out.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
        wr[0]   = std::move(routed);
        wr.publish(1UZ);
    }

    void processMessages(gr::MsgPortIn& port, std::span<const gr::Message> messages) {
        if (!rate_engine.has_value()) [[unlikely]] {
            start();
        }
        if (!rate_engine.has_value()) {
            return;
        }
        if (std::addressof(port) == std::addressof(quality_metric_in)) {
            for (const gr::Message& msg : messages) {
                if (!msg.data.has_value()) [[unlikely]] {
                    continue;
                }
                const gr::property_map& m   = msg.data.value();
                const float              snr = detail::float_from_map(m, std::string_view{"snr_db"}).value_or(0.F);
                const float ber =
                    detail::float_from_map(m, std::string_view{"ber"}).value_or(0.F);
                const float q =
                    detail::float_from_map(m, std::string_view{"quality"}).value_or(0.F);
                const detail::ModulationMode before = rate_engine->current_mode();
                const bool                     switched = rate_engine->update_quality(snr, ber, q);
                if (!switched) {
                    continue;
                }
                const detail::ModulationMode now_mode = rate_engine->current_mode();
                publish_mode_change_tensors(before, now_mode);
                publish_kiss_wire(detail::kiss_cmd_mode_ch,
                                  detail::encode_mode_change(station_identifier.value, now_mode));
            }
            return;
        }

        if (std::addressof(port) == std::addressof(negotiation_pdu_in)) {
            const std::vector<detail::ModulationMode> supported_modes = detail::parse_csv_modes(supported_modes_csv.value);

            for (const gr::Message& msg : messages) {
                if (!msg.data.has_value()) [[unlikely]] {
                    continue;
                }
                const gr::property_map& body = msg.data.value();

                std::optional<std::uint64_t> cmd_u64;
                if (const auto it = body.find(gr::convert_string_domain(std::string_view{"kiss_command"}));
                    it != body.end()) [[likely]] {
                    if (const auto* pu = it->second.get_if<std::uint64_t>()) {
                        cmd_u64 = *pu;
                    } else if (const auto* ps = it->second.get_if<std::int64_t>()) {
                        cmd_u64 = static_cast<std::uint64_t>(*ps);
                    }
                }
                if (!cmd_u64.has_value()) [[unlikely]] {
                    continue;
                }
                const auto* pay = detail::tensor_bytes_from_map(body, std::string_view{"kiss_payload"});
                if (pay == nullptr) [[unlikely]] {
                    continue;
                }
                detail::dispatch_negotiation_command(
                    station_identifier.value,
                    supported_modes,
                    negotiated_mode,
                    negotiated_peers,
                    static_cast<std::uint8_t>(*cmd_u64),
                    pay->data(),
                    pay->size(),
                    [this](std::uint8_t c, std::vector<std::uint8_t>&& pb) { publish_kiss_wire(c, std::move(pb)); });
            }
        }
    }
};

} // namespace gnuradio4::packet_protocols

#endif

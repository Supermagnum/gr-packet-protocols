// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_MODULATION_NEGOTIATION_HPP
#define GNURADIO4_PACKET_PROTOCOLS_MODULATION_NEGOTIATION_HPP

#include <gnuradio-4.0/packet_protocols/detail/Helpers.hpp>
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
#include <unordered_map>
#include <utility>
#include <vector>

namespace gnuradio4::packet_protocols {

GR_REGISTER_BLOCK(gnuradio4::packet_protocols::ModulationNegotiation)

struct ModulationNegotiation : gr::Block<ModulationNegotiation, gr::NoTagPropagation> {
    using Description =
        gr::Doc<"State machine companion for negotiation frames (ported from modulation_negotiation_impl, message-only NOTIFY transport).">;

    gr::MsgPortIn  negotiation_pdu_in{};
    gr::MsgPortOut negotiation_pdu_out{};

    gr::Annotated<std::string, "station_identifier">   station_identifier{std::string("GROUND")};
    gr::Annotated<std::string, "supported_modes_csv"> supported_modes_csv{std::string("0,1,5,7")};

    GR_MAKE_REFLECTABLE(ModulationNegotiation, negotiation_pdu_in, negotiation_pdu_out, station_identifier, supported_modes_csv);

    detail::ModulationMode negotiated_mode{detail::ModulationMode::mode_4fsk};
    std::unordered_map<std::string, detail::ModulationMode> negotiated_peers{};

    void start() noexcept {}

    [[nodiscard]] gr::work::Status processBulk() noexcept { return gr::work::Status::OK; }

    void publish_pdu(std::uint8_t kiss_cmd, std::vector<std::uint8_t>&& payload_octets) {
        gr::property_map pb;
        pb[gr::convert_string_domain(std::string_view{"kiss_command"})] =
            gr::pmt::Value(static_cast<std::uint64_t>(kiss_cmd));
        pb[gr::convert_string_domain(std::string_view{"kiss_payload"})] =
            gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(payload_octets)));
        gr::Message m{};
        m.cmd  = gr::message::Command::Notify;
        m.data = std::move(pb);
        auto wr = negotiation_pdu_out.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
        wr[0]   = std::move(m);
        wr.publish(1UZ);
    }

    void processMessages(gr::MsgPortIn& port, std::span<const gr::Message> messages) {
        if (std::addressof(port) != std::addressof(negotiation_pdu_in)) [[unlikely]] {
            return;
        }
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
            const std::uint8_t cmd_byte = static_cast<std::uint8_t>(*cmd_u64);

            constexpr std::string_view pay_key{"kiss_payload"};
            const auto*                pay = detail::tensor_bytes_from_map(body, pay_key);
            if (pay == nullptr) [[unlikely]] {
                continue;
            }
            const std::uint8_t* data = pay->data();
            const std::size_t   len  = pay->size();

            detail::dispatch_negotiation_command(
                station_identifier.value,
                supported_modes,
                negotiated_mode,
                negotiated_peers,
                cmd_byte,
                data,
                len,
                [this](std::uint8_t c, std::vector<std::uint8_t>&& pb) { publish_pdu(c, std::move(pb)); });
        }
    }
};

} // namespace gnuradio4::packet_protocols

#endif

// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_KISS_TNC_HPP
#define GNURADIO4_PACKET_PROTOCOLS_KISS_TNC_HPP

#include <gnuradio-4.0/packet_protocols/detail/Helpers.hpp>
#include <gnuradio-4.0/packet_protocols/detail/KissFraming.hpp>
#include <gnuradio-4.0/packet_protocols/detail/ProtocolConstants.hpp>

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/Value.hpp>
#include <gnuradio-4.0/annotated.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace gnuradio4::packet_protocols {

GR_REGISTER_BLOCK(gnuradio4::packet_protocols::KissTnc)

/*! Logical KISS deframer/framer suitable for PDU message ports (serial device I/O intentionally omitted). */
struct KissTnc : gr::Block<KissTnc, gr::NoTagPropagation> {
    using Description = gr::Doc<"Encapsulates payloads into escaped KISS frames and extracts payloads from inbound KISS frames.">;

    gr::MsgPortIn  pdu_into_kiss_in{};
    gr::MsgPortOut kiss_wire_out{};
    gr::MsgPortIn  kiss_wire_in{};
    gr::MsgPortOut pdu_from_kiss_out{};

    gr::Annotated<std::uint8_t, "kiss_command_byte"> kiss_command_byte{detail::kiss_cmd_data};

    GR_MAKE_REFLECTABLE(KissTnc, pdu_into_kiss_in, kiss_wire_out, kiss_wire_in, pdu_from_kiss_out, kiss_command_byte);

    void start() noexcept {}

    [[nodiscard]] gr::work::Status processBulk() noexcept { return gr::work::Status::OK; }

    void processMessages(gr::MsgPortIn& port, std::span<const gr::Message> messages) {
        constexpr std::string_view k{"pdu_bytes"};
        if (std::addressof(port) == std::addressof(pdu_into_kiss_in)) {
            for (const gr::Message& msg : messages) {
                if (!msg.data.has_value()) {
                    continue;
                }
                const auto* in = detail::tensor_bytes_from_map(msg.data.value(), k);
                if (in == nullptr) {
                    continue;
                }
                std::vector<std::uint8_t> payload(in->begin(), in->end());
                std::vector<std::uint8_t> wire =
                    detail::kissFrame(static_cast<std::uint8_t>(kiss_command_byte.value), std::move(payload));
                gr::property_map pb;
                pb[gr::convert_string_domain(k)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(wire));
                gr::Message r{};
                r.cmd  = gr::message::Command::Notify;
                r.data = std::move(pb);
                auto ws = kiss_wire_out.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
                ws[0]   = std::move(r);
                ws.publish(1UZ);
            }
        } else if (std::addressof(port) == std::addressof(kiss_wire_in)) {
            for (const gr::Message& msg : messages) {
                if (!msg.data.has_value()) {
                    continue;
                }
                const auto* kin = detail::tensor_bytes_from_map(msg.data.value(), k);
                if (kin == nullptr || kin->size() < 2UZ) {
                    continue;
                }
                std::vector<std::uint8_t> frame(kin->begin(), kin->end());
                auto decoded = detail::kissDeframeChunk(frame);
                if (!decoded.has_value()) {
                    continue;
                }
                const std::uint8_t      cmd_byte = decoded->first;
                std::vector<std::uint8_t> body   = std::move(decoded->second);
                gr::property_map pb;
                pb[gr::convert_string_domain(std::string_view("kiss_command"))] =
                    gr::pmt::Value(static_cast<std::uint64_t>(cmd_byte));
                pb[gr::convert_string_domain(k)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(body)));
                gr::Message r{};
                r.cmd  = gr::message::Command::Notify;
                r.data = std::move(pb);
                auto ws = pdu_from_kiss_out.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
                ws[0]   = std::move(r);
                ws.publish(1UZ);
            }
        }
    }
};

} // namespace gnuradio4::packet_protocols

#endif

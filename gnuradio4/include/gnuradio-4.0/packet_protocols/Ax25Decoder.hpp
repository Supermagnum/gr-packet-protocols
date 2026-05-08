// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_AX25_DECODER_HPP
#define GNURADIO4_PACKET_PROTOCOLS_AX25_DECODER_HPP

#include <gnuradio-4.0/packet_protocols/detail/Ax25Protocol.hpp>
#include <gnuradio-4.0/packet_protocols/detail/Helpers.hpp>

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Tag.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/Value.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace gnuradio4::packet_protocols {

GR_REGISTER_BLOCK(gnuradio4::packet_protocols::Ax25Decoder)

struct Ax25Decoder : gr::Block<Ax25Decoder, gr::NoTagPropagation> {
    using Description = gr::Doc<"Parses AX.25 UI frames produced by Ax25Encoder, validates FCS, outputs the information field PDU.">;

    gr::MsgPortIn  pdu_in{};
    gr::MsgPortOut pdu_out{};

    GR_MAKE_REFLECTABLE(Ax25Decoder, pdu_in, pdu_out);

    void start() noexcept {}

    [[nodiscard]] gr::work::Status processBulk() noexcept { return gr::work::Status::OK; }

    void processMessages(gr::MsgPortIn& port, std::span<const gr::Message> messages) {
        if (std::addressof(port) != std::addressof(pdu_in)) [[unlikely]] {
            return;
        }
        constexpr std::string_view pdu_key{"pdu_bytes"};
        for (const gr::Message& message : messages) {
            if (!message.data.has_value()) {
                continue;
            }
            const gr::property_map&         body    = message.data.value();
            const gr::Tensor<std::uint8_t>* frame   = detail::tensor_bytes_from_map(body, pdu_key);
            if (frame == nullptr || frame->size() < 14UZ) {
                continue;
            }
            ax25_frame_t parsed{};
            const int parse_rc = ax25_parse_frame(frame->data(), static_cast<std::uint16_t>(frame->size()), &parsed);
            if (parse_rc != 0 || ax25_validate_frame(&parsed) != 0) {
                continue;
            }
            if (!ax25_check_fcs(frame->data(), static_cast<std::uint16_t>(frame->size()), parsed.fcs)) {
                continue;
            }
            const std::span<const std::uint8_t> info(parsed.info, parsed.info_length);
            std::vector<std::uint8_t> payload(info.begin(), info.end());

            gr::property_map out_body;
            out_body[gr::convert_string_domain(pdu_key)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(payload)));
            gr::Message      out{};
            out.cmd  = gr::message::Command::Notify;
            out.data = std::move(out_body);
            auto ws = pdu_out.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
            ws[0]   = std::move(out);
            ws.publish(1UZ);
        }
    }
};

} // namespace gnuradio4::packet_protocols

#endif

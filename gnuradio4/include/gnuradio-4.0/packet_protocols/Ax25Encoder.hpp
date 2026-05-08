// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_AX25_ENCODER_HPP
#define GNURADIO4_PACKET_PROTOCOLS_AX25_ENCODER_HPP

#include <gnuradio-4.0/packet_protocols/detail/Ax25Protocol.hpp>
#include <gnuradio-4.0/packet_protocols/detail/Helpers.hpp>

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Tag.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/Value.hpp>
#include <gnuradio-4.0/annotated.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace gnuradio4::packet_protocols {

GR_REGISTER_BLOCK(gnuradio4::packet_protocols::Ax25Encoder)

struct Ax25Encoder : gr::Block<Ax25Encoder, gr::NoTagPropagation> {
    using Description =
        gr::Doc<"Builds AX.25 UI frames from PDU byte tensors using the bundled AX.25 C implementation (no HDLC opening flags added).">;

    gr::MsgPortIn  pdu_in{};
    gr::MsgPortOut pdu_out{};

    gr::Annotated<std::string, "callsign_src"> callsign_src{std::string("NOCALL")};
    gr::Annotated<std::string, "callsign_dst"> callsign_dst{std::string("NOCALL")};
    gr::Annotated<int, "ssid_src">              ssid_src{0};
    gr::Annotated<int, "ssid_dst">              ssid_dst{0};
    gr::Annotated<bool, "extended_mode"> extended_mode{
        false}; //!< Reserved API knob (UI framing path does not activate modulo-128 sequencing).

    GR_MAKE_REFLECTABLE(Ax25Encoder, pdu_in, pdu_out, callsign_src, callsign_dst, ssid_src, ssid_dst, extended_mode);

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
            const gr::property_map&         body = message.data.value();
            const gr::Tensor<std::uint8_t>* pdu = detail::tensor_bytes_from_map(body, pdu_key);
            if (pdu == nullptr || pdu->empty()) {
                continue;
            }
            ax25_address_t dst{};
            ax25_address_t src{};
            if (ax25_set_address(&dst, callsign_dst.value.c_str(),
                                 static_cast<std::uint8_t>(ssid_dst.value & 0x0F),
                                 true) != 0) {
                continue;
            }
            if (ax25_set_address(&src, callsign_src.value.c_str(),
                                 static_cast<std::uint8_t>(ssid_src.value & 0x0F),
                                 false) != 0) {
                continue;
            }
            ax25_frame_t frame{};
            if (ax25_create_frame(&frame, &src, &dst, AX25_CTRL_UI, AX25_PID_NONE, pdu->data(),
                                  static_cast<std::uint16_t>(pdu->size()))
                != 0) {
                continue;
            }
            std::array<std::uint8_t, 4096U> encoded{};
            std::uint16_t encoded_len = static_cast<std::uint16_t>(encoded.size());
            if (ax25_encode_frame(&frame, encoded.data(), &encoded_len) != 0) {
                continue;
            }
            std::vector<std::uint8_t> wire(
                encoded.begin(), encoded.begin() + static_cast<std::ptrdiff_t>(encoded_len));
            gr::property_map out_body;
            out_body[gr::convert_string_domain(pdu_key)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(wire));
            gr::Message      out{};
            out.cmd  = gr::message::Command::Notify;
            out.data = std::move(out_body);
            auto ws  = pdu_out.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
            ws[0]    = std::move(out);
            ws.publish(1UZ);
        }
    }
};

} // namespace gnuradio4::packet_protocols

#endif

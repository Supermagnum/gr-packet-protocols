// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_FX25_DECODER_HPP
#define GNURADIO4_PACKET_PROTOCOLS_FX25_DECODER_HPP

#include <gnuradio-4.0/packet_protocols/detail/FxIl2pCodec.hpp>
#include <gnuradio-4.0/packet_protocols/detail/Helpers.hpp>

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Tag.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/Value.hpp>
#include <gnuradio-4.0/annotated.hpp>

#include <cstddef>
#include <vector>

namespace gnuradio4::packet_protocols {

GR_REGISTER_BLOCK(gnuradio4::packet_protocols::Fx25Decoder)

struct Fx25Decoder : gr::Block<Fx25Decoder, gr::NoTagPropagation> {
    using Description = gr::Doc<"Inverse of Fx25Encoder: removes FEC wrapper and restores AX.25 frame bytes (byte-oriented).">;

    gr::MsgPortIn  pdu_in{};
    gr::MsgPortOut pdu_out{};

    gr::Annotated<bool, "expect_crc16_footer"> expect_crc16_footer{true};

    GR_MAKE_REFLECTABLE(Fx25Decoder, pdu_in, pdu_out, expect_crc16_footer);

    void start() noexcept {}

    [[nodiscard]] gr::work::Status processBulk() noexcept { return gr::work::Status::OK; }

    void processMessages(gr::MsgPortIn& port, std::span<const gr::Message> messages) {
        if (std::addressof(port) != std::addressof(pdu_in)) [[unlikely]] {
            return;
        }
        constexpr std::string_view key{"pdu_bytes"};
        for (const gr::Message& msg : messages) {
            if (!msg.data.has_value()) {
                continue;
            }
            const gr::Tensor<std::uint8_t>* tin = detail::tensor_bytes_from_map(msg.data.value(), key);
            if (tin == nullptr || tin->size() < 8UZ) {
                continue;
            }
            std::vector<std::uint8_t> frame(tin->begin(), tin->end());
            const auto inner = detail::decodeFx25Bytes(frame, expect_crc16_footer.value);
            if (!inner.has_value()) {
                continue;
            }
            gr::property_map body;
            body[gr::convert_string_domain(key)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(inner.value()));
            gr::Message      out{};
            out.cmd                              = gr::message::Command::Notify;
            out.data                             = std::move(body);
            auto ws = pdu_out.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
            ws[0]   = std::move(out);
            ws.publish(1UZ);
        }
    }
};

} // namespace gnuradio4::packet_protocols

#endif

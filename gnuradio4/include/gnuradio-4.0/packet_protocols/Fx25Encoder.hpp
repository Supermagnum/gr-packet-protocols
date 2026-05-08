// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_FX25_ENCODER_HPP
#define GNURADIO4_PACKET_PROTOCOLS_FX25_ENCODER_HPP

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
#include <cstdint>
#include <string>

namespace gnuradio4::packet_protocols {

GR_REGISTER_BLOCK(gnuradio4::packet_protocols::Fx25Encoder)

struct Fx25Encoder : gr::Block<Fx25Encoder, gr::NoTagPropagation> {
    using Description = gr::Doc<"Wraps an AX.25 frame (bytes) inside an FX.25 Reed-Solomon container. Message-only block.">;

    gr::MsgPortIn  pdu_in{};
    gr::MsgPortOut pdu_out{};

    gr::Annotated<int, "fec_type">            fec_type{static_cast<int>(detail::fx25_fec_rs255_223)};
    gr::Annotated<int, "interleaver_depth">     interleaver_depth{1};
    gr::Annotated<bool, "add_crc16_footer"> add_crc16_footer{
        true}; //!< Two-byte checksum matching the FX.25 reference implementation bundled with GR3-style sources.

    GR_MAKE_REFLECTABLE(Fx25Encoder, pdu_in, pdu_out, fec_type, interleaver_depth, add_crc16_footer);

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
            const gr::property_map& map = msg.data.value();
            const auto* tin           = detail::tensor_bytes_from_map(map, key);
            if (tin == nullptr || tin->empty()) {
                continue;
            }
            std::vector<std::uint8_t> inner(tin->begin(), tin->end());
            const auto wired = detail::encodeFx25Bytes(inner, static_cast<std::uint8_t>(fec_type.value & 0xFF),
                                                        static_cast<std::uint8_t>(interleaver_depth.value <= 0
                                                                                       ? 1
                                                                                       : interleaver_depth.value),
                                                        add_crc16_footer.value);
            if (!wired.has_value()) {
                continue;
            }
            gr::property_map body;
            body[gr::convert_string_domain(key)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(wired.value()));
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

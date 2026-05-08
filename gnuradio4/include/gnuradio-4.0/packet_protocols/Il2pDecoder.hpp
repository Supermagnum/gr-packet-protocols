// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_IL2P_DECODER_HPP
#define GNURADIO4_PACKET_PROTOCOLS_IL2P_DECODER_HPP

#include <gnuradio-4.0/packet_protocols/detail/FxIl2pCodec.hpp>
#include <gnuradio-4.0/packet_protocols/detail/Helpers.hpp>

#include <gnuradio-4.0/Block.hpp>
#include <gnuradio-4.0/BlockRegistry.hpp>
#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/Value.hpp>
#include <gnuradio-4.0/annotated.hpp>

namespace gnuradio4::packet_protocols {

GR_REGISTER_BLOCK(gnuradio4::packet_protocols::Il2pDecoder)

struct Il2pDecoder : gr::Block<Il2pDecoder, gr::NoTagPropagation> {
    using Description = gr::Doc<"Decodes scrambled IL2P frames back to PDUs using the bundled RS implementation.">;

    gr::MsgPortIn  pdu_in{};
    gr::MsgPortOut pdu_out{};

    gr::Annotated<bool, "expect_crc32_footer"> expect_crc32_footer{true};

    GR_MAKE_REFLECTABLE(Il2pDecoder, pdu_in, pdu_out, expect_crc32_footer);

    void start() noexcept {}

    [[nodiscard]] gr::work::Status processBulk() noexcept { return gr::work::Status::OK; }

    void processMessages(gr::MsgPortIn& port, std::span<const gr::Message> messages) {
        if (std::addressof(port) != std::addressof(pdu_in)) [[unlikely]] {
            return;
        }
        constexpr std::string_view k{"pdu_bytes"};
        for (const gr::Message& msg : messages) {
            if (!msg.data.has_value()) {
                continue;
            }
            const auto* in = detail::tensor_bytes_from_map(msg.data.value(), k);
            if (in == nullptr || in->empty()) {
                continue;
            }
            std::vector<std::uint8_t> wired(in->begin(), in->end());
            const auto pay = detail::decodeIl2pBytes(wired, expect_crc32_footer.value);
            if (!pay.has_value()) {
                continue;
            }
            gr::property_map pb;
            pb[gr::convert_string_domain(k)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(pay.value()));
            gr::Message r{};
            r.cmd  = gr::message::Command::Notify;
            r.data = std::move(pb);
            auto ws = pdu_out.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
            ws[0]   = std::move(r);
            ws.publish(1UZ);
        }
    }
};

} // namespace gnuradio4::packet_protocols

#endif

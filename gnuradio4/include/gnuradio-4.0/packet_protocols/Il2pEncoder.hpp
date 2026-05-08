// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_IL2P_ENCODER_HPP
#define GNURADIO4_PACKET_PROTOCOLS_IL2P_ENCODER_HPP

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

GR_REGISTER_BLOCK(gnuradio4::packet_protocols::Il2pEncoder)

struct Il2pEncoder : gr::Block<Il2pEncoder, gr::NoTagPropagation> {
    using Description = gr::Doc<"IL2P framing with scrambling, preamble, sync word and Reed-Solomon payload blocks (byte tensors).">;

    gr::MsgPortIn  pdu_in{};
    gr::MsgPortOut pdu_out{};

    gr::Annotated<std::string, "dest_callsign"> dest_callsign{std::string("NOCALL")};
    gr::Annotated<std::string, "dest_ssid">     dest_ssid{std::string("0")};
    gr::Annotated<std::string, "src_callsign">  src_callsign{std::string("NOCALL")};
    gr::Annotated<std::string, "src_ssid">      src_ssid{std::string("0")};
    gr::Annotated<int, "fec_type">              fec_type{static_cast<int>(detail::il2p_fec_rs255_223)};
    gr::Annotated<bool, "add_crc32_footer"> add_crc32_footer{true};

    GR_MAKE_REFLECTABLE(Il2pEncoder, pdu_in, pdu_out, dest_callsign, dest_ssid, src_callsign, src_ssid, fec_type, add_crc32_footer);

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
            std::vector<std::uint8_t> payload(in->begin(), in->end());
            const auto outv = detail::encodeIl2pBytes(payload, dest_callsign.value, dest_ssid.value, src_callsign.value,
                                                        src_ssid.value, static_cast<std::uint8_t>(fec_type.value & 0xFF),
                                                        add_crc32_footer.value);
            if (!outv.has_value()) {
                continue;
            }
            gr::property_map pb;
            pb[gr::convert_string_domain(k)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(outv.value()));
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

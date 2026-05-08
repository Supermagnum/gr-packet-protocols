// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/ut.hpp>

#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Sequence.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/packet_protocols/KissTnc.hpp>
#include <gnuradio-4.0/packet_protocols/detail/ProtocolConstants.hpp>

#include <string_view>
#include <utility>
#include <vector>

using namespace boost::ut;

namespace {

constexpr std::string_view kPdu{"pdu_bytes"};

void push(gr::MsgPortOut& upstream, std::vector<std::uint8_t> payload) {
    gr::property_map body;
    body[gr::convert_string_domain(kPdu)] =
        gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(payload)));
    gr::Message message;
    message.cmd = gr::message::Command::Notify;
    message.data = std::move(body);
    auto span = upstream.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    span[0] = std::move(message);
    span.publish(1UZ);
}

[[nodiscard]] std::vector<std::uint8_t> grab_wire(gr::MsgPortIn& in) {
    expect(eq(in.streamReader().available(), 1UZ));
    auto rd = in.streamReader().template get<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    const auto* t =
        rd[0].data.value().at(gr::convert_string_domain(kPdu)).get_if<gr::Tensor<std::uint8_t>>();
    expect(t != nullptr);
    std::vector<std::uint8_t> out(t->data(), t->data() + t->size());
    expect(rd.consume(rd.size()));
    return out;
}

[[nodiscard]] std::vector<std::uint8_t>
grab_payload_and_cmd(gr::MsgPortIn& in, std::uint64_t expected_cmd, std::string_view payload_key,
                     std::string_view cmd_key) {
    expect(eq(in.streamReader().available(), 1UZ));
    auto rd = in.streamReader().template get<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    const auto& map = rd[0].data.value();
    const auto cmd  = *(map.at(gr::convert_string_domain(cmd_key)).get_if<std::uint64_t>());
    expect(eq(cmd, expected_cmd));
    const auto* t = map.at(gr::convert_string_domain(payload_key)).get_if<gr::Tensor<std::uint8_t>>();
    expect(t != nullptr);
    std::vector<std::uint8_t> out(t->data(), t->data() + t->size());
    expect(rd.consume(rd.size()));
    return out;
}

} // namespace

static suite KissSuite = [] {
    "kiss wire braces FEND escapes and survives round trip"_test = [] {
        gnuradio4::packet_protocols::KissTnc k{};
        k.init(std::make_shared<gr::Sequence>());
        k.start();

        gr::MsgPortOut to_kiss{};
        gr::MsgPortIn  from_kiss_wire{};
        gr::MsgPortOut loopback{};
        gr::MsgPortIn  pdu_return{};

        expect(to_kiss.connect(k.pdu_into_kiss_in).has_value());
        expect(k.kiss_wire_out.connect(from_kiss_wire).has_value());
        expect(loopback.connect(k.kiss_wire_in).has_value());
        expect(k.pdu_from_kiss_out.connect(pdu_return).has_value());

        std::vector<std::uint8_t> hostile{gnuradio4::packet_protocols::detail::kiss_fend,
                                            gnuradio4::packet_protocols::detail::kiss_fesc,
                                            gnuradio4::packet_protocols::detail::kiss_tfend};
        push(to_kiss, hostile);
        k.processScheduledMessages();
        auto wire = grab_wire(from_kiss_wire);
        expect(eq(wire.front(), gnuradio4::packet_protocols::detail::kiss_fend));
        expect(eq(wire.back(), gnuradio4::packet_protocols::detail::kiss_fend));

        push(loopback, wire);
        k.processScheduledMessages();
        auto recovered = grab_payload_and_cmd(pdu_return, gnuradio4::packet_protocols::detail::kiss_cmd_data,
                                              kPdu, std::string_view{"kiss_command"});
        expect(recovered == hostile);
    };
};

int main() { return boost::ut::cfg<boost::ut::override>.run(); }

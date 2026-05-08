// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/ut.hpp>

#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Sequence.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/packet_protocols/Ax25Decoder.hpp>
#include <gnuradio-4.0/packet_protocols/Ax25Encoder.hpp>

#include <string_view>
#include <vector>

using namespace boost::ut;

namespace {

void notify_pdu(gr::MsgPortOut& port, std::vector<std::uint8_t> octets, std::string_view key_view) {
    gr::property_map body;
    body[gr::convert_string_domain(key_view)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(octets)));
    gr::Message m{};
    m.cmd  = gr::message::Command::Notify;
    m.data = std::move(body);
    auto ws = port.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    ws[0]   = std::move(m);
    ws.publish(1UZ);
}

[[nodiscard]] std::vector<std::uint8_t> ax25_encode_bytes(const std::vector<std::uint8_t>& payload) {
    constexpr std::string_view k{"pdu_bytes"};
    gnuradio4::packet_protocols::Ax25Encoder enc{};
    enc.callsign_dst.value.assign("APRS");
    enc.callsign_src.value.assign("KQ4FOO");
    enc.ssid_dst.value = 1;
    enc.ssid_src.value = 11;
    enc.init(std::make_shared<gr::Sequence>());
    enc.start();

    gr::MsgPortOut up{};
    gr::MsgPortIn  out{};
    expect(up.connect(enc.pdu_in).has_value());
    expect(enc.pdu_out.connect(out).has_value());
    notify_pdu(up, payload, k);
    enc.processScheduledMessages();
    expect(eq(out.streamReader().available(), 1UZ));
    auto r = out.streamReader().template get<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    const gr::Tensor<std::uint8_t>* wire =
        r[0].data.value().at(gr::convert_string_domain(k)).get_if<gr::Tensor<std::uint8_t>>();
    expect(wire != nullptr);
    std::vector<std::uint8_t> copy(wire->data(), wire->data() + wire->size());
    expect(r.consume(r.size()));
    return copy;
}

[[nodiscard]] std::vector<std::uint8_t> ax25_decode_bytes(const std::vector<std::uint8_t>& wire) {
    constexpr std::string_view k{"pdu_bytes"};
    gnuradio4::packet_protocols::Ax25Decoder dec{};
    dec.init(std::make_shared<gr::Sequence>());
    dec.start();
    gr::MsgPortOut up{};
    gr::MsgPortIn  out{};
    expect(up.connect(dec.pdu_in).has_value());
    expect(dec.pdu_out.connect(out).has_value());
    notify_pdu(up, wire, k);
    dec.processScheduledMessages();
    expect(eq(out.streamReader().available(), 1UZ));
    auto r = out.streamReader().template get<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    const gr::Tensor<std::uint8_t>* pay =
        r[0].data.value().at(gr::convert_string_domain(k)).get_if<gr::Tensor<std::uint8_t>>();
    expect(pay != nullptr);
    std::vector<std::uint8_t> copy(pay->data(), pay->data() + pay->size());
    expect(r.consume(r.size()));
    return copy;
}

} // namespace

static suite Ax25Decode = [] {
    "round trip recovers payload and invalid FCS is dropped"_test = [] {
        const std::vector<std::uint8_t> payload{0x10, 0x20, 0x30};
        std::vector<std::uint8_t>       wire = ax25_encode_bytes(payload);
        const auto                      back = ax25_decode_bytes(wire);
        expect(back == payload);

        expect(wire.size() >= 3UZ);
        wire.back() ^= std::uint8_t{0x04}; // perturb FCS
        gnuradio4::packet_protocols::Ax25Decoder dec{};
        dec.init(std::make_shared<gr::Sequence>());
        dec.start();
        gr::MsgPortOut up{};
        gr::MsgPortIn  rd{};
        expect(up.connect(dec.pdu_in).has_value());
        expect(dec.pdu_out.connect(rd).has_value());
        notify_pdu(up, wire, std::string_view{"pdu_bytes"});
        dec.processScheduledMessages();
        expect(eq(rd.streamReader().available(), 0UZ));
    };
};

int main() { return boost::ut::cfg<boost::ut::override>.run(); }

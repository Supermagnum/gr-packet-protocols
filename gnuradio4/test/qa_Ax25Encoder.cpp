// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/ut.hpp>

#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Sequence.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/packet_protocols/Ax25Encoder.hpp>

#include <string_view>
#include <vector>

using namespace boost::ut;

namespace {

void notify_pdu(gr::MsgPortOut& port, const std::vector<std::uint8_t>& octets, std::string_view key_view) {
    gr::property_map body;
    std::vector<std::uint8_t> copy(octets.begin(), octets.end());
    body[gr::convert_string_domain(key_view)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(copy)));
    gr::Message m{};
    m.cmd  = gr::message::Command::Notify;
    m.data = std::move(body);
    auto ws = port.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    ws[0]   = std::move(m);
    ws.publish(1UZ);
}

} // namespace

static suite Ax25Encode = [] {
    "Ax25Encoder emits longer AX.25 frame than payload"_test = [] {
        gnuradio4::packet_protocols::Ax25Encoder enc{};
        enc.callsign_dst.value.assign("APRS");
        enc.callsign_src.value.assign("KQ4FOO");
        enc.ssid_dst.value = 1;
        enc.ssid_src.value = 11;
        enc.init(std::make_shared<gr::Sequence>());
        enc.start();

        gr::MsgPortOut to_enc{};
        gr::MsgPortIn  from_enc{};
        expect(to_enc.connect(enc.pdu_in).has_value());
        expect(enc.pdu_out.connect(from_enc).has_value());

        std::vector<std::uint8_t> payload{'a', 'b', 'c'};
        constexpr std::string_view k{"pdu_bytes"};
        notify_pdu(to_enc, payload, k);
        enc.processScheduledMessages();

        expect(eq(from_enc.streamReader().available(), 1UZ));
        auto r = from_enc.streamReader().template get<gr::SpanReleasePolicy::ProcessAll>(1UZ);
        expect(static_cast<bool>(r[0].data.has_value()));
        const gr::property_map&         body = r[0].data.value();
        const gr::Tensor<std::uint8_t>* wire =
            body.at(gr::convert_string_domain(k)).get_if<gr::Tensor<std::uint8_t>>();
        expect(wire != nullptr);
        expect(wire->size() > payload.size());
        expect(r.consume(r.size()));
    };
};

int main() { return boost::ut::cfg<boost::ut::override>.run(); }

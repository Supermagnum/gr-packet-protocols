// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/ut.hpp>

#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Sequence.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/packet_protocols/Il2pEncoder.hpp>

#include <utility>
#include <vector>

using namespace boost::ut;

namespace {

constexpr std::string_view k{"pdu_bytes"};

void notify_pdu(gr::MsgPortOut& upstream, std::vector<std::uint8_t> payload) {
    gr::property_map body;
    body[gr::convert_string_domain(k)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(payload)));
    gr::Message m{};
    m.cmd  = gr::message::Command::Notify;
    m.data = std::move(body);
    auto ws = upstream.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    ws[0]   = std::move(m);
    ws.publish(1UZ);
}

} // namespace

static suite Il2pEncSuite = [] {
    "encoder inserts IL2P preamble and sync"_test = [] {
        gnuradio4::packet_protocols::Il2pEncoder enc{};
        enc.dest_callsign.value.assign("MYCALL");
        enc.dest_ssid.value.assign("0");
        enc.src_callsign.value.assign("HISCALL");
        enc.src_ssid.value.assign("1");
        enc.init(std::make_shared<gr::Sequence>());
        enc.start();

        gr::MsgPortOut up{};
        gr::MsgPortIn  rd{};
        expect(up.connect(enc.pdu_in).has_value());
        expect(enc.pdu_out.connect(rd).has_value());
        notify_pdu(up, {0xBE, 0xEF});
        enc.processScheduledMessages();
        auto r       = rd.streamReader().template get<gr::SpanReleasePolicy::ProcessAll>(1UZ);
        const auto* t = r[0].data.value().at(gr::convert_string_domain(k)).get_if<gr::Tensor<std::uint8_t>>();
        expect(t != nullptr);
        expect(eq(t->data()[0], std::uint8_t{0x55}));
        expect(eq(t->data()[1], std::uint8_t{0xF1}));
        expect(eq(t->data()[2], std::uint8_t{0x5E}));
        expect(eq(t->data()[3], std::uint8_t{0x48}));
        expect(r.consume(r.size()));
    };
};

int main() { return boost::ut::cfg<boost::ut::override>.run(); }

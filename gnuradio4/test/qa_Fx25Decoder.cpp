// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/ut.hpp>

#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Sequence.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/packet_protocols/Ax25Encoder.hpp>
#include <gnuradio-4.0/packet_protocols/Fx25Decoder.hpp>
#include <gnuradio-4.0/packet_protocols/Fx25Encoder.hpp>

#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

using namespace boost::ut;

namespace {

constexpr std::string_view pdu_key{"pdu_bytes"};

void notify_pdu(gr::MsgPortOut& port, std::vector<std::uint8_t> octets) {
    gr::property_map body;
    body[gr::convert_string_domain(pdu_key)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(octets)));
    gr::Message msg;
    msg.cmd  = gr::message::Command::Notify;
    msg.data = std::move(body);
    auto ws  = port.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    ws[0]    = std::move(msg);
    ws.publish(1UZ);
}

[[nodiscard]] std::vector<std::uint8_t> read_pdu(gr::MsgPortIn& from) {
    expect(eq(from.streamReader().available(), 1UZ));
    auto r = from.streamReader().template get<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    const auto* t =
        r[0].data.value().at(gr::convert_string_domain(pdu_key)).get_if<gr::Tensor<std::uint8_t>>();
    expect(t != nullptr);
    std::vector<std::uint8_t> out(t->data(), t->data() + t->size());
    expect(r.consume(r.size()));
    return out;
}

[[nodiscard]] std::vector<std::uint8_t> make_inner_ax25() {
    gnuradio4::packet_protocols::Ax25Encoder ax{};
    ax.callsign_dst.value.assign("APRS");
    ax.callsign_src.value.assign("KQ4FOO");
    ax.ssid_dst.value = 1;
    ax.ssid_src.value = 9;
    ax.init(std::make_shared<gr::Sequence>());
    ax.start();
    gr::MsgPortOut upstream{};
    gr::MsgPortIn  reader{};
    expect(upstream.connect(ax.pdu_in).has_value());
    expect(ax.pdu_out.connect(reader).has_value());
    notify_pdu(upstream, {0xC0});
    ax.processScheduledMessages();
    return read_pdu(reader);
}

} // namespace

static suite FxDecoderSuite = [] {
    "encode then decode survives one flipped byte"_test = [] {
        const std::vector<std::uint8_t> inner = make_inner_ax25();

        gnuradio4::packet_protocols::Fx25Encoder fxenc{};
        gnuradio4::packet_protocols::Fx25Decoder fxdec{};
        fxenc.add_crc16_footer.value    = false;
        fxdec.expect_crc16_footer.value = false;
        fxenc.init(std::make_shared<gr::Sequence>());
        fxdec.init(std::make_shared<gr::Sequence>());
        fxenc.start();
        fxdec.start();

        gr::MsgPortOut up{};
        gr::MsgPortIn  mid{};
        expect(up.connect(fxenc.pdu_in).has_value());
        expect(fxenc.pdu_out.connect(mid).has_value());
        notify_pdu(up, inner);
        fxenc.processScheduledMessages();

        auto frame = read_pdu(mid);
        expect(frame.size() > inner.size());

        frame[frame.size() / 2] ^= std::uint8_t{0x41};

        gr::MsgPortOut up2{};
        gr::MsgPortIn  healed_reader{};
        expect(up2.connect(fxdec.pdu_in).has_value());
        expect(fxdec.pdu_out.connect(healed_reader).has_value());
        notify_pdu(up2, std::move(frame));
        fxdec.processScheduledMessages();
        auto back = read_pdu(healed_reader);
        expect(back.size() >= inner.size());
        expect(std::equal(inner.begin(), inner.end(), back.begin()));
    };
};

int main() { return boost::ut::cfg<boost::ut::override>.run(); }

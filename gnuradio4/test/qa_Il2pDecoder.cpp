// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/ut.hpp>

#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Sequence.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/packet_protocols/Il2pDecoder.hpp>
#include <gnuradio-4.0/packet_protocols/Il2pEncoder.hpp>

#include <algorithm>
#include <string_view>
#include <vector>

using namespace boost::ut;

namespace {

constexpr std::string_view k{"pdu_bytes"};

void notify(gr::MsgPortOut& up, std::vector<std::uint8_t> data) {
    gr::property_map body;
    body[gr::convert_string_domain(k)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(data)));
    gr::Message msg;
    msg.cmd  = gr::message::Command::Notify;
    msg.data = std::move(body);
    auto ws = up.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    ws[0]   = std::move(msg);
    ws.publish(1UZ);
}

[[nodiscard]] std::vector<std::uint8_t> read_one(gr::MsgPortIn& from) {
    expect(eq(from.streamReader().available(), 1UZ));
    auto r = from.streamReader().template get<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    auto* tens = r[0].data.value().at(gr::convert_string_domain(k)).get_if<gr::Tensor<std::uint8_t>>();
    expect(tens != nullptr);
    std::vector<std::uint8_t> out(tens->data(), tens->data() + tens->size());
    expect(r.consume(r.size()));
    return out;
}

} // namespace

static suite Il2pRound = [] {
    "decoder recovers tampered-but-correctable frame"_test = [] {
        gnuradio4::packet_protocols::Il2pEncoder enc{};
        gnuradio4::packet_protocols::Il2pDecoder dec{};
        enc.dest_callsign.value.assign("MYCALL");
        enc.dest_ssid.value.assign("0");
        enc.src_callsign.value.assign("OTHER");
        enc.src_ssid.value.assign("1");
        enc.add_crc32_footer.value    = false;
        dec.expect_crc32_footer.value = false;
        enc.init(std::make_shared<gr::Sequence>());
        dec.init(std::make_shared<gr::Sequence>());
        enc.start();
        dec.start();

        std::vector<std::uint8_t> payload{'i', 'l', '2', 'p'};
        gr::MsgPortOut            up{};
        gr::MsgPortIn             mid{};
        expect(up.connect(enc.pdu_in).has_value());
        expect(enc.pdu_out.connect(mid).has_value());
        notify(up, payload);
        enc.processScheduledMessages();
        auto frame = read_one(mid);

        frame[frame.size() / 2] ^= std::uint8_t{0x07};

        gr::MsgPortOut up2{};
        gr::MsgPortIn  healed{};
        expect(up2.connect(dec.pdu_in).has_value());
        expect(dec.pdu_out.connect(healed).has_value());
        notify(up2, std::move(frame));
        dec.processScheduledMessages();
        auto back = read_one(healed);
        expect(back.size() >= payload.size());
        expect(std::equal(payload.begin(), payload.end(), back.begin()));
    };
};

int main() { return boost::ut::cfg<boost::ut::override>.run(); }

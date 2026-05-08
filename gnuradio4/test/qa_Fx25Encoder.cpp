// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/ut.hpp>
#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Sequence.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/packet_protocols/Ax25Encoder.hpp>
#include <gnuradio-4.0/packet_protocols/Fx25Encoder.hpp>
#include <string_view>
#include <vector>

using namespace boost::ut;

namespace {

void notify(gr::MsgPortOut& port, std::vector<std::uint8_t> data, std::string_view key) {
  gr::property_map body;
  body[gr::convert_string_domain(key)] = gr::pmt::Value(gr::Tensor<std::uint8_t>(std::move(data)));
  gr::Message msg;
  msg.cmd = gr::message::Command::Notify;
  msg.data = std::move(body);
  auto span = port.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
  span[0] = std::move(msg);
  span.publish(1UZ);
}

std::vector<std::uint8_t> encode_ax25(const std::vector<std::uint8_t>& payload) {
  constexpr std::string_view key{"pdu_bytes"};
  gnuradio4::packet_protocols::Ax25Encoder enc{};
  enc.callsign_dst.value.assign("APRS");
  enc.callsign_src.value.assign("KQ4FOO");
  enc.ssid_dst.value = 2;
  enc.ssid_src.value = 3;
  enc.init(std::make_shared<gr::Sequence>());
  enc.start();
  gr::MsgPortOut upstream;
  gr::MsgPortIn reader;
  expect(upstream.connect(enc.pdu_in).has_value());
  expect(enc.pdu_out.connect(reader).has_value());
  notify(upstream, payload, key);
  enc.processScheduledMessages();
  expect(eq(reader.streamReader().available(), 1UZ));
  auto r = reader.streamReader().template get<gr::SpanReleasePolicy::ProcessAll>(1UZ);
  const auto* tens = r[0].data.value().at(gr::convert_string_domain(key)).get_if<gr::Tensor<std::uint8_t>>();
  expect(tens != nullptr);
  std::vector<std::uint8_t> out(tens->data(), tens->data() + tens->size());
  expect(r.consume(r.size()));
  return out;
}

} // namespace

static suite FxEncode = [] {
  "Fx wrapper enlarges AX.25 frame"_test = [] {
    constexpr std::string_view key{"pdu_bytes"};
    auto inner = encode_ax25({0x01, 0x02, 0x03});

    gnuradio4::packet_protocols::Fx25Encoder fx{};
    fx.init(std::make_shared<gr::Sequence>());
    fx.start();

    gr::MsgPortOut up{};
    gr::MsgPortIn out{};
    expect(up.connect(fx.pdu_in).has_value());
    expect(fx.pdu_out.connect(out).has_value());
    notify(up, inner, key);
    fx.processScheduledMessages();
    expect(eq(out.streamReader().available(), 1UZ));
    auto r = out.streamReader().template get<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    const auto* tens = r[0].data.value().at(gr::convert_string_domain(key)).get_if<gr::Tensor<std::uint8_t>>();
    expect(tens != nullptr);
    expect(tens->size() > inner.size());
    expect(eq(static_cast<unsigned>(tens->data()[1]), unsigned('F')));
    expect(r.consume(r.size()));
  };
};

int main() { return boost::ut::cfg<boost::ut::override>.run(); }

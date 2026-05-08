// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/ut.hpp>

#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Sequence.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/packet_protocols/AdaptiveRateControl.hpp>

#include <string_view>

using namespace boost::ut;

namespace {

void push_quality(gr::MsgPortOut& downstream, float snr, float ber, float q) {
    gr::property_map body;
    body[gr::convert_string_domain(std::string_view{"snr_db"})]  = gr::pmt::Value(snr);
    body[gr::convert_string_domain(std::string_view{"ber"})]    = gr::pmt::Value(ber);
    body[gr::convert_string_domain(std::string_view{"quality"})] = gr::pmt::Value(q);
    gr::Message message;
    message.cmd  = gr::message::Command::Notify;
    message.data = std::move(body);
    auto span    = downstream.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    span[0]      = std::move(message);
    span.publish(1UZ);
}

const gr::Tensor<std::uint8_t>* tensor_mode(const gr::property_map& body, std::string_view key) {
    const gr::pmt::Value& v = body.at(gr::convert_string_domain(key));
    return v.get_if<gr::Tensor<std::uint8_t>>();
}

} // namespace

static suite AdaptiveRateSuite = [] {
    "quality slump drops high-order modulation"_test = [] {
        gnuradio4::packet_protocols::AdaptiveRateControl block(
            gr::property_map{{std::pmr::string("adaptation_seed_mode"),
                              gr::pmt::Value(static_cast<std::uint8_t>(7))}});
        block.init(std::make_shared<gr::Sequence>());
        block.start();

        gr::MsgPortOut upstream;
        gr::MsgPortIn  mode_reader;
        expect(upstream.connect(block.quality_metric_in).has_value());
        expect(block.mode_change_out.connect(mode_reader).has_value());

        push_quality(upstream, 3.F, 5e-3F, 0.05F);
        block.processScheduledMessages();

        expect(eq(mode_reader.streamReader().available(), 1UZ));
        auto r = mode_reader.streamReader().template get<gr::SpanReleasePolicy::ProcessAll>(1UZ);
        expect(static_cast<bool>(r[0].data.has_value()));
        const gr::property_map& body = r[0].data.value();
        const auto*             prev_t = tensor_mode(body, std::string_view{"previous_modulation_mode"});
        const auto*             cur_t  = tensor_mode(body, std::string_view{"current_modulation_mode"});
        expect(prev_t != nullptr && cur_t != nullptr);
        expect(eq(prev_t->front(), std::uint8_t{7}));
        expect(eq(cur_t->front(), std::uint8_t{0}));
        expect(r.consume(r.size()));
    };
};

int main() { return boost::ut::cfg<boost::ut::override>.run(); }

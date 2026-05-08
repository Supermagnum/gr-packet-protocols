// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/ut.hpp>

#include <gnuradio-4.0/Message.hpp>
#include <gnuradio-4.0/Port.hpp>
#include <gnuradio-4.0/Sequence.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/packet_protocols/LinkQualityMonitor.hpp>

#include <string_view>

using namespace boost::ut;

namespace {

void fire(gr::MsgPortOut& up) {
    gr::property_map noop;
    gr::Message msg;
    msg.cmd  = gr::message::Command::Notify;
    msg.data = noop;
    auto span =
        up.streamWriter().template reserve<gr::SpanReleasePolicy::ProcessAll>(1UZ);
    span[0] = std::move(msg);
    span.publish(1UZ);
}

} // namespace

static suite Link = [] {
    "annotated thresholds drive passed flag"_test = [] {
        gnuradio4::packet_protocols::LinkQualityMonitor mon(
            gr::property_map{{std::pmr::string{"snr_db_measurement"}, gr::pmt::Value(12.F)},
                             {std::pmr::string{"ber_measurement"}, gr::pmt::Value(0.001F)},
                             {std::pmr::string{"fer_measurement"}, gr::pmt::Value(0.01F)},
                             {std::pmr::string{"snr_threshold_db"}, gr::pmt::Value(10.F)},
                             {std::pmr::string{"ber_threshold"}, gr::pmt::Value(0.01F)},
                             {std::pmr::string{"fer_threshold"}, gr::pmt::Value(0.02F)}});

        mon.init(std::make_shared<gr::Sequence>());
        mon.start();

        gr::MsgPortOut up{};
        gr::MsgPortIn  rd{};
        expect(up.connect(mon.observe_in).has_value());
        expect(mon.quality_metric_out.connect(rd).has_value());

        fire(up);
        mon.processScheduledMessages();
        auto rs = rd.streamReader().template get<gr::SpanReleasePolicy::ProcessAll>(1UZ);
        const auto* pass =
            rs[0].data.value().at(gr::convert_string_domain(std::string_view{"passed_thresholds"})).get_if<gr::Tensor<std::uint8_t>>();
        expect(pass != nullptr);
        expect(eq(pass->front(), std::uint8_t{1}));
        expect(rs.consume(rs.size()));
    };
};

int main() { return boost::ut::cfg<boost::ut::override>.run(); }

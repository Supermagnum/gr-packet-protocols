// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/ut.hpp>

#include <gnuradio-4.0/packet_protocols/detail/Ax25Protocol.hpp>

#include <array>

using namespace boost::ut;

static suite AxProto = [] {
    "UI encode + parse honours FCS"_test = [] {
        ax25_tnc_t tnc{};
        expect(eq(ax25_init(&tnc), 0));

        ax25_address_t dst{};
        ax25_address_t src{};
        expect(eq(ax25_set_address(&dst, "APRSS", uint8_t{1}, true), 0));
        expect(eq(ax25_set_address(&src, "KQ4FOO", uint8_t{2}, false), 0));

        std::array<std::uint8_t, 4> payload{10, 20, 30, 40};
        ax25_frame_t frame{};
        expect(eq(ax25_create_frame(&frame, &src, &dst, AX25_CTRL_UI, AX25_PID_NONE, payload.data(),
                                    static_cast<std::uint16_t>(payload.size())),
                  0));

        std::array<std::uint8_t, 1024> wire{};
        std::uint16_t                  wire_len =
            static_cast<std::uint16_t>(wire.size());
        expect(eq(ax25_encode_frame(&frame, wire.data(), &wire_len), 0));

        ax25_frame_t parsed{};
        expect(eq(ax25_parse_frame(wire.data(), wire_len, &parsed), 0));
        expect(static_cast<bool>(ax25_check_fcs(wire.data(), wire_len, parsed.fcs)));
        expect(ax25_cleanup(&tnc) == 0);
    };
};

int main() { return boost::ut::cfg<boost::ut::override>.run(); }

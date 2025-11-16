/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 *
 * gr-packet-protocols is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * gr-packet-protocols is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gr-packet-protocols; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include <gnuradio/packet_protocols/ax25_encoder.h>
#include <gnuradio/packet_protocols/ax25_decoder.h>
#include <gnuradio/packet_protocols/fx25_encoder.h>
#include <gnuradio/packet_protocols/fx25_decoder.h>
#include <gnuradio/packet_protocols/il2p_encoder.h>
#include <gnuradio/packet_protocols/il2p_decoder.h>
#include <gnuradio/packet_protocols/kiss_tnc.h>
#include <gnuradio/packet_protocols/common.h>

namespace py = pybind11;

PYBIND11_MODULE(packet_protocols_python, m) {
    m.doc() = "gr-packet-protocols Python bindings";

    // AX.25 Encoder - register class (hidden), then module-level function
    py::class_<gr::packet_protocols::ax25_encoder,
               std::shared_ptr<gr::packet_protocols::ax25_encoder>>(
        m, "_ax25_encoder_impl");

    m.def("ax25_encoder",
          &gr::packet_protocols::ax25_encoder::make,
          py::arg("dest_callsign"),
          py::arg("dest_ssid"),
          py::arg("src_callsign"),
          py::arg("src_ssid"),
          py::arg("digipeaters") = "",
          py::arg("command_response") = false,
          py::arg("poll_final") = false,
          "Create an AX.25 encoder block");

    // AX.25 Decoder - register class (hidden), then module-level function
    py::class_<gr::packet_protocols::ax25_decoder,
               std::shared_ptr<gr::packet_protocols::ax25_decoder>>(
        m, "_ax25_decoder_impl");

    m.def("ax25_decoder",
          &gr::packet_protocols::ax25_decoder::make,
          "Create an AX.25 decoder block");

    // FX.25 Encoder - class with methods, then module-level function
    auto fx25_encoder_class = py::class_<gr::packet_protocols::fx25_encoder,
                                         std::shared_ptr<gr::packet_protocols::fx25_encoder>>(
        m, "_fx25_encoder_impl")
        .def("set_fec_type", &gr::packet_protocols::fx25_encoder::set_fec_type)
        .def("set_interleaver_depth", &gr::packet_protocols::fx25_encoder::set_interleaver_depth)
        .def("set_add_checksum", &gr::packet_protocols::fx25_encoder::set_add_checksum);

    m.def("fx25_encoder",
          &gr::packet_protocols::fx25_encoder::make,
          py::arg("fec_type") = FX25_FEC_RS_16_12,
          py::arg("interleaver_depth") = 1,
          py::arg("add_checksum") = true,
          py::return_value_policy::reference_internal,
          "Create an FX.25 encoder block");

    // FX.25 Decoder - register class (hidden), then module-level function
    py::class_<gr::packet_protocols::fx25_decoder,
               std::shared_ptr<gr::packet_protocols::fx25_decoder>>(
        m, "_fx25_decoder_impl");

    m.def("fx25_decoder",
          &gr::packet_protocols::fx25_decoder::make,
          "Create an FX.25 decoder block");

    // IL2P Encoder - class with methods, then module-level function
    auto il2p_encoder_class = py::class_<gr::packet_protocols::il2p_encoder,
                                          std::shared_ptr<gr::packet_protocols::il2p_encoder>>(
        m, "_il2p_encoder_impl")
        .def("set_fec_type", &gr::packet_protocols::il2p_encoder::set_fec_type)
        .def("set_add_checksum", &gr::packet_protocols::il2p_encoder::set_add_checksum);

    m.def("il2p_encoder",
          &gr::packet_protocols::il2p_encoder::make,
          py::arg("dest_callsign"),
          py::arg("dest_ssid"),
          py::arg("src_callsign"),
          py::arg("src_ssid"),
          py::arg("fec_type") = IL2P_FEC_RS_255_223,
          py::arg("add_checksum") = true,
          py::return_value_policy::reference_internal,
          "Create an IL2P encoder block");

    // IL2P Decoder - register class (hidden), then module-level function
    py::class_<gr::packet_protocols::il2p_decoder,
               std::shared_ptr<gr::packet_protocols::il2p_decoder>>(
        m, "_il2p_decoder_impl");

    m.def("il2p_decoder",
          &gr::packet_protocols::il2p_decoder::make,
          "Create an IL2P decoder block");

    // KISS TNC - class with methods, then module-level function
    auto kiss_tnc_class = py::class_<gr::packet_protocols::kiss_tnc,
                                     std::shared_ptr<gr::packet_protocols::kiss_tnc>>(
        m, "_kiss_tnc_impl")
        .def("set_tx_delay", &gr::packet_protocols::kiss_tnc::set_tx_delay)
        .def("set_persistence", &gr::packet_protocols::kiss_tnc::set_persistence)
        .def("set_slot_time", &gr::packet_protocols::kiss_tnc::set_slot_time)
        .def("set_tx_tail", &gr::packet_protocols::kiss_tnc::set_tx_tail)
        .def("set_full_duplex", &gr::packet_protocols::kiss_tnc::set_full_duplex);

    m.def("kiss_tnc",
          &gr::packet_protocols::kiss_tnc::make,
          py::arg("device"),
          py::arg("baud_rate") = 9600,
          py::arg("hardware_flow_control") = false,
          py::return_value_policy::reference_internal,
          "Create a KISS TNC block");
}

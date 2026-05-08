// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_DETAIL_HELPERS_HPP
#define GNURADIO4_PACKET_PROTOCOLS_DETAIL_HELPERS_HPP

#include <gnuradio-4.0/Tag.hpp>
#include <gnuradio-4.0/Tensor.hpp>
#include <gnuradio-4.0/Value.hpp>

#include <cstddef>
#include <memory_resource>
#include <optional>
#include <string_view>

namespace gnuradio4::packet_protocols::detail {

[[nodiscard]] inline const gr::Tensor<std::uint8_t>* tensor_bytes_from_map(const gr::property_map& map,
                                                                           std::string_view key_view) {
    const std::pmr::string key(key_view.begin(), key_view.end());
    const auto               it = map.find(key);
    if (it == map.end()) {
        return nullptr;
    }
    return it->second.get_if<gr::Tensor<std::uint8_t>>();
}

[[nodiscard]] inline std::optional<float> float_from_map(const gr::property_map& map, std::string_view key_view) {
    const std::pmr::string key(key_view.begin(), key_view.end());
    const auto               it = map.find(key);
    if (it == map.end()) {
        return std::nullopt;
    }
    if (const float* p = it->second.get_if<float>()) {
        return *p;
    }
    if (const double* p = it->second.get_if<double>()) {
        return static_cast<float>(*p);
    }
    return std::nullopt;
}

} // namespace gnuradio4::packet_protocols::detail

#endif

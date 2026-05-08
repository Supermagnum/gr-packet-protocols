// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GNURADIO4_PACKET_PROTOCOLS_DETAIL_ADAPTIVE_RATE_ENGINE_HPP
#define GNURADIO4_PACKET_PROTOCOLS_DETAIL_ADAPTIVE_RATE_ENGINE_HPP

#include <gnuradio-4.0/packet_protocols/detail/ModulationMode.hpp>

#include <map>
#include <vector>

namespace gnuradio4::packet_protocols::detail {

struct RateThresholds final {
    float snr_min_dB{};
    float snr_max_dB{};
    float ber_max{};
    float quality_min{};
};

/*! Ported from adaptive_rate_control_impl (GNU Radio 3.10 sources in this repo). Used by GR4 adaptive blocks. */
class AdaptiveRateEngine final {
    ModulationMode                      _current;
    ModulationMode                      _last{};
    bool                                _adapt{};
    float                               _hysteresis{};
    bool                                _tier4{};
    float                               _last_snr{};
    std::map<ModulationMode, RateThresholds> _mode_thresholds;
    std::map<ModulationMode, int>            _mode_data_rates;

    void initialize_thresholds();

    [[nodiscard]] RateThresholds thresholds_for(ModulationMode mode) const noexcept;

public:
    explicit AdaptiveRateEngine(ModulationMode initial_mode,
                                bool            enable_adaptation,
                                float           hysteresis_db,
                                bool            enable_tier4);

    [[nodiscard]] ModulationMode current_mode() const noexcept { return _current; }
    [[nodiscard]] ModulationMode last_mode_before_change() const noexcept { return _last; }

    void set_adaptation_enabled(bool enabled) noexcept { _adapt = enabled; }
    void set_tier4_enabled(bool enabled) noexcept;

    void set_modulation_mode(ModulationMode mode) noexcept;

    [[nodiscard]] ModulationMode recommend_mode(float snr_db, float ber) const noexcept;

    /*! Applies one quality observation. Returns true if the modulation mode changed. */
    bool update_quality(float snr_db, float ber, float quality_score) noexcept;

    [[nodiscard]] int get_data_rate() const noexcept;
};

inline AdaptiveRateEngine::AdaptiveRateEngine(ModulationMode initial_mode,
                                             bool enable_adaptation,
                                             float hysteresis_db,
                                             bool enable_tier4)
    : _current(initial_mode), _last(initial_mode), _adapt(enable_adaptation), _hysteresis(hysteresis_db), _tier4(enable_tier4) {
    if (isTier4(initial_mode) && !_tier4) {
        _current = ModulationMode::mode_2fsk;
    }
    initialize_thresholds();
}

inline void AdaptiveRateEngine::set_tier4_enabled(bool enabled) noexcept {
    _tier4 = enabled;
    if (!_tier4 && isTier4(_current)) {
        _current = ModulationMode::mode_2fsk;
        _last    = ModulationMode::mode_2fsk;
    }
}

inline void AdaptiveRateEngine::set_modulation_mode(ModulationMode mode) noexcept {
    if (isTier4(mode) && !_tier4) {
        return;
    }
    _current = mode;
    _last    = mode;
}

inline RateThresholds AdaptiveRateEngine::thresholds_for(ModulationMode mode) const noexcept {
    const auto it = _mode_thresholds.find(mode);
    if (it != _mode_thresholds.end()) {
        return it->second;
    }
    return {0.F, 15.F, 0.01F, 0.3F};
}

inline void AdaptiveRateEngine::initialize_thresholds() {
    using M = ModulationMode;
    _mode_thresholds[M::mode_2fsk]         = {0.F, 15.F, 0.01F, 0.3F};
    _mode_data_rates[M::mode_2fsk]         = 1200;
    _mode_thresholds[M::mode_4fsk]         = {8.F, 20.F, 0.005F, 0.5F};
    _mode_data_rates[M::mode_4fsk]         = 2400;
    _mode_thresholds[M::mode_8fsk]        = {12.F, 25.F, 0.001F, 0.7F};
    _mode_data_rates[M::mode_8fsk]         = 3600;
    _mode_thresholds[M::mode_16fsk]        = {18.F, 30.F, 0.0005F, 0.8F};
    _mode_data_rates[M::mode_16fsk]        = 4800;
    _mode_thresholds[M::mode_bpsk]         = {6.F, 18.F, 0.01F, 0.4F};
    _mode_data_rates[M::mode_bpsk]        = 1200;
    _mode_thresholds[M::mode_qpsk]         = {10.F, 22.F, 0.005F, 0.6F};
    _mode_data_rates[M::mode_qpsk]         = 2400;
    _mode_thresholds[M::mode_8psk]         = {14.F, 26.F, 0.001F, 0.75F};
    _mode_data_rates[M::mode_8psk]        = 3600;
    _mode_thresholds[M::mode_qam16]       = {16.F, 28.F, 0.0005F, 0.8F};
    _mode_data_rates[M::mode_qam16]      = 9600;
    _mode_thresholds[M::mode_qam64_6250] = {20.F, 32.F, 0.0001F, 0.85F};
    _mode_data_rates[M::mode_qam64_6250] = 37500;
    _mode_thresholds[M::mode_qam64_12500] = {22.F, 35.F, 0.0001F, 0.9F};
    _mode_data_rates[M::mode_qam64_12500] = 75000;
    _mode_thresholds[M::mode_qam256]      = {28.F, 40.F, 0.00005F, 0.95F};
    _mode_data_rates[M::mode_qam256]      = 100000;
    _mode_thresholds[M::mode_bpsk_12500]  = {8.F, 20.F, 0.005F, 0.5F};
    _mode_data_rates[M::mode_bpsk_12500]  = 12500;
    _mode_thresholds[M::mode_qpsk_12500]  = {12.F, 24.F, 0.002F, 0.65F};
    _mode_data_rates[M::mode_qpsk_12500]  = 25000;
    _mode_thresholds[M::mode_8psk_12500]  = {16.F, 28.F, 0.0008F, 0.78F};
    _mode_data_rates[M::mode_8psk_12500] = 37500;
    _mode_thresholds[M::mode_qam16_12500] = {18.F, 30.F, 0.0003F, 0.82F};
    _mode_data_rates[M::mode_qam16_12500] = 50000;
    _mode_thresholds[M::mode_soqpsk_1m]   = {10.F, 25.F, 0.001F, 0.6F};
    _mode_data_rates[M::mode_soqpsk_1m] = 1000000;
    _mode_thresholds[M::mode_soqpsk_5m]   = {15.F, 30.F, 0.0005F, 0.7F};
    _mode_data_rates[M::mode_soqpsk_5m] = 5000000;
    _mode_thresholds[M::mode_soqpsk_10m]  = {18.F, 33.F, 0.0003F, 0.75F};
    _mode_data_rates[M::mode_soqpsk_10m] = 10000000;
    _mode_thresholds[M::mode_soqpsk_20m]  = {22.F, 36.F, 0.0002F, 0.8F};
    _mode_data_rates[M::mode_soqpsk_20m] = 20000000;
    _mode_thresholds[M::mode_soqpsk_40m] = {26.F, 40.F, 0.0001F, 0.85F};
    _mode_data_rates[M::mode_soqpsk_40m] = 40000000;
}

inline ModulationMode AdaptiveRateEngine::recommend_mode(float snr_db, float ber) const noexcept {
    ModulationMode best_mode = ModulationMode::mode_2fsk;
    int               best_rate  = 0;
    using M                      = ModulationMode;
    const std::vector<ModulationMode> modes{
        M::mode_soqpsk_40m,   M::mode_soqpsk_20m,  M::mode_soqpsk_10m, M::mode_soqpsk_5m,     M::mode_soqpsk_1m,
        M::mode_qam256,       M::mode_qam64_12500, M::mode_qam16_12500, M::mode_8psk_12500,    M::mode_qpsk_12500,
        M::mode_bpsk_12500,   M::mode_qam64_6250,  M::mode_qam16,       M::mode_16fsk,         M::mode_8psk,
        M::mode_8fsk,         M::mode_qpsk,        M::mode_4fsk,        M::mode_bpsk,          M::mode_2fsk};
    for (auto mode : modes) {
        if (isTier4(mode) && !_tier4) continue;
        const RateThresholds t = thresholds_for(mode);
        auto                 rate_it = _mode_data_rates.find(mode);
        if (rate_it == _mode_data_rates.end()) continue;
        if (snr_db >= t.snr_min_dB && snr_db <= t.snr_max_dB && ber <= t.ber_max) {
            if (rate_it->second > best_rate) {
                best_mode = mode;
                best_rate = rate_it->second;
            }
        }
    }
    return best_mode;
}

inline bool AdaptiveRateEngine::update_quality(float snr_db, float ber, float quality_score) noexcept {
    if (!_adapt) [[unlikely]] {
        return false;
    }
    _last_snr = snr_db;
    auto current_thresholds = thresholds_for(_current);
    ModulationMode         recommended = _current;
    if ((snr_db > (current_thresholds.snr_max_dB + _hysteresis) && ber < current_thresholds.ber_max && quality_score > current_thresholds.quality_min)) {
        recommended = recommend_mode(snr_db, ber);
    } else if (snr_db < (current_thresholds.snr_min_dB - _hysteresis) || ber > current_thresholds.ber_max || quality_score < (current_thresholds.quality_min - 0.2F)) {
        recommended = recommend_mode(snr_db, ber);
    }
    if (recommended != _current) {
        _last    = _current;
        _current = recommended;
        return true;
    }
    return false;
}

inline int AdaptiveRateEngine::get_data_rate() const noexcept {
    const auto it = _mode_data_rates.find(_current);
    if (it != _mode_data_rates.end()) {
        return it->second;
    }
    return 1200;
}

} // namespace gnuradio4::packet_protocols::detail

#endif

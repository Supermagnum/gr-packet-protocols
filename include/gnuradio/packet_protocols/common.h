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

#ifndef INCLUDED_PACKET_PROTOCOLS_COMMON_H
#define INCLUDED_PACKET_PROTOCOLS_COMMON_H

#include <algorithm>
#include <vector>
#include <cstdint>

// AX.25 Constants
#define AX25_FLAG 0x7E
#define AX25_FRAME_MIN_SIZE 18

// KISS TNC Constants
#define KISS_FEND 0xC0
#define KISS_FESC 0xDB
#define KISS_TFEND 0xDC
#define KISS_TFESC 0xDD

#define KISS_CMD_DATA 0x00
#define KISS_CMD_TXDELAY 0x01
#define KISS_CMD_P 0x02
#define KISS_CMD_SLOTTIME 0x03
#define KISS_CMD_TXTAIL 0x04
#define KISS_CMD_FULLDUPLEX 0x05
#define KISS_CMD_SET_HARDWARE 0x06
#define KISS_CMD_NEG_REQ 0x10      // Negotiation request
#define KISS_CMD_NEG_RESP 0x11     // Negotiation response
#define KISS_CMD_NEG_ACK 0x12       // Negotiation acknowledgment
#define KISS_CMD_MODE_CHANGE 0x13  // Mode change notification
#define KISS_CMD_QUALITY_FB 0x14   // Quality feedback
#define KISS_CMD_RETURN 0xFF

// FX.25 FEC Types (Reed-Solomon codes matching Dire Wolf and FX.25 spec)
#define FX25_FEC_RS_255_239 0x01  // Reed-Solomon (255,239) - 16 parity bytes
#define FX25_FEC_RS_255_223 0x02  // Reed-Solomon (255,223) - 32 parity bytes
#define FX25_FEC_RS_255_191 0x03  // Reed-Solomon (255,191) - 64 parity bytes
#define FX25_FEC_RS_255_159 0x04  // Reed-Solomon (255,159) - 96 parity bytes
#define FX25_FEC_RS_255_127 0x05  // Reed-Solomon (255,127) - 128 parity bytes
#define FX25_FEC_RS_255_95 0x06   // Reed-Solomon (255,95) - 160 parity bytes
#define FX25_FEC_RS_255_63 0x07   // Reed-Solomon (255,63) - 192 parity bytes
#define FX25_FEC_RS_255_31 0x08   // Reed-Solomon (255,31) - 224 parity bytes

// IL2P FEC Types
#define IL2P_FEC_RS_255_223 0x01
#define IL2P_FEC_RS_255_239 0x02
#define IL2P_FEC_RS_255_247 0x03

// Galois Field GF(256) arithmetic for Reed-Solomon
// Primitive polynomial: x^8 + x^4 + x^3 + x^2 + 1 = 0x11D
class GaloisField256 {
  private:
    static const int GF_SIZE = 256;
    static const int PRIMITIVE_POLY = 0x11D; // x^8 + x^4 + x^3 + x^2 + 1
    static inline uint8_t s_alpha_to[GF_SIZE]{};
    static inline uint8_t s_index_of[GF_SIZE]{};
    static inline bool s_tables_ready{};

    static void ensure_tables()
    {
        if (s_tables_ready)
            return;

        s_index_of[0] = 0xFF;
        s_alpha_to[255] = 0;

        uint8_t sr = 1;
        for (int i = 0; i < 255; i++) {
            s_alpha_to[i] = sr;
            s_index_of[sr] = static_cast<uint8_t>(i);
            unsigned u = static_cast<unsigned>(sr) << 1;
            if (u & 0x100u)
                u ^= static_cast<unsigned>(PRIMITIVE_POLY);
            sr = static_cast<uint8_t>(u & 0xFFu);
        }

        s_tables_ready = true;
    }

  public:
    GaloisField256() { ensure_tables(); }

    uint8_t multiply(uint8_t a, uint8_t b) {
        ensure_tables();
        if (a == 0 || b == 0) return 0;
        int sum = (int)s_index_of[a] + (int)s_index_of[b];
        if (sum >= 255) sum -= 255;
        return s_alpha_to[sum];
    }

    uint8_t divide(uint8_t a, uint8_t b) {
        ensure_tables();
        if (a == 0) return 0;
        if (b == 0) return 0; // Division by zero
        int diff = (int)s_index_of[a] - (int)s_index_of[b];
        if (diff < 0) diff += 255;
        return s_alpha_to[diff];
    }

    uint8_t power(uint8_t a, int n) {
        ensure_tables();
        if (a == 0) return 0;
        if (n == 0) return 1;
        int exp = ((int)s_index_of[a] * n) % 255;
        return s_alpha_to[exp];
    }

    uint8_t add(uint8_t a, uint8_t b) {
        return a ^ b; // Addition in GF(2^8) is XOR
    }

    uint8_t subtract(uint8_t a, uint8_t b) {
        return a ^ b; // Subtraction in GF(2^8) is same as addition
    }

    static unsigned gf_modnn(unsigned x)
    {
        while (x >= 255u) {
            x -= 255u;
            x = (x >> 8u) + (x & 255u);
        }
        return x;
    }

    unsigned gf_index_of(uint8_t x) const
    {
        ensure_tables();
        if (x == 0)
            return 255;
        return s_index_of[x];
    }

    uint8_t gf_alpha_to(unsigned idx) const
    {
        ensure_tables();
        idx = gf_modnn(idx);
        if (idx == 255u)
            return 0;
        return s_alpha_to[idx];
    }
};

// Reed-Solomon Codec Classes
class ReedSolomonEncoder {
  private:
    int d_n;  // Code length (255)
    int d_k;  // Data length
    int d_t;  // Error correction capability = (n-k)/2
    GaloisField256 d_gf;
    std::vector<uint8_t> d_generator_poly;
    /** init_rs / encode_rs: generator coefficients in index form (log), A0=255 for zero poly coeff */
    std::vector<uint8_t> d_generator_index;

    void build_generator_poly() {
        /* Same recurrence as Phil Karn init_rs.c (genpoly), roots alpha^{FCR*PRIM + i*PRIM}. */
        const unsigned FCR = 1;
        const unsigned PRIM = 1;
        const int nroots = 2 * d_t;

        d_generator_poly.resize(static_cast<size_t>(nroots + 1));
        d_generator_poly[0] = 1;
        for (int i = 0, root = static_cast<int>(FCR * PRIM); i < nroots;
             i++, root += static_cast<int>(PRIM)) {
            d_generator_poly[static_cast<size_t>(i + 1)] = 1;
            for (int j = i; j > 0; j--) {
                if (d_generator_poly[static_cast<size_t>(j)] != 0)
                    d_generator_poly[static_cast<size_t>(j)] =
                        d_gf.add(d_generator_poly[static_cast<size_t>(j - 1)],
                                 d_gf.gf_alpha_to(GaloisField256::gf_modnn(
                                     d_gf.gf_index_of(d_generator_poly[static_cast<size_t>(j)]) +
                                     static_cast<unsigned>(root))));
                else
                    d_generator_poly[static_cast<size_t>(j)] =
                        d_generator_poly[static_cast<size_t>(j - 1)];
            }
            d_generator_poly[0] = d_gf.gf_alpha_to(GaloisField256::gf_modnn(
                d_gf.gf_index_of(d_generator_poly[0]) + static_cast<unsigned>(root)));
        }

        d_generator_index.resize(static_cast<size_t>(nroots + 1));
        for (int i = 0; i <= nroots; i++) {
            uint8_t c = d_generator_poly[static_cast<size_t>(i)];
            d_generator_index[static_cast<size_t>(i)] =
                c ? static_cast<uint8_t>(d_gf.gf_index_of(c)) : static_cast<uint8_t>(255);
        }
    }

  public:
    ReedSolomonEncoder(int n, int k) : d_n(n), d_k(k), d_t((n - k) / 2) {
        if (n != 255) {
            // Only support RS(255,k) codes
            d_n = 255;
            d_k = k;
            d_t = (d_n - d_k) / 2;
        }
        build_generator_poly();
    }
    
    ~ReedSolomonEncoder() = default;

    std::vector<uint8_t> encode(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> result(d_n, 0);

        int data_len = (data.size() < (size_t)d_k) ? static_cast<int>(data.size()) : d_k;
        for (int i = 0; i < data_len; i++) {
            result[static_cast<size_t>(i)] = data[static_cast<size_t>(i)];
        }
        for (int i = data_len; i < d_k; i++) {
            result[static_cast<size_t>(i)] = 0;
        }

        /* Phil Karn encode_rs.h LFSR (genpoly in index form); NN=255, PAD implied 0 */
        const unsigned NROOTS = static_cast<unsigned>(2 * d_t);
        const unsigned NN = static_cast<unsigned>(d_n);
        const unsigned A0 = NN;

        std::vector<uint8_t> bb(NROOTS, 0);
        for (unsigned i = 0; i < NN - NROOTS; i++) {
            unsigned feedback =
                static_cast<unsigned>(d_gf.gf_index_of(d_gf.add(result[i], bb[0])));
            if (feedback != A0) {
                for (unsigned j = 1; j < NROOTS; j++) {
                    bb[j] = d_gf.add(bb[j],
                                     d_gf.gf_alpha_to(GaloisField256::gf_modnn(
                                         feedback + d_generator_index[NROOTS - j])));
                }
            }
            for (unsigned j = 0; j < NROOTS - 1; j++) {
                bb[j] = bb[j + 1];
            }
            if (feedback != A0) {
                bb[NROOTS - 1] =
                    d_gf.gf_alpha_to(GaloisField256::gf_modnn(feedback + d_generator_index[0]));
            } else {
                bb[NROOTS - 1] = 0;
            }
        }
        for (unsigned j = 0; j < NROOTS; j++) {
            result[d_k + static_cast<int>(j)] = bb[j];
        }

        return result;
    }
    
    int get_data_length() const {
        return d_k;
    }
    int get_code_length() const {
        return d_n;
    }
    int get_error_correction_capability() const {
        return d_t;
    }
};

class ReedSolomonDecoder {
  private:
    int d_n;  // Code length (255)
    int d_k;  // Data length
    int d_t;  // Error correction capability = (n-k)/2
    GaloisField256 d_gf;

    // Calculate syndromes
    bool calculate_syndromes(const std::vector<uint8_t>& received, std::vector<uint8_t>& syndromes) {
        syndromes.resize(2 * d_t);
        bool has_errors = false;
        
        // Syndrome S_i = R(alpha^(i+1)) for i = 0 to 2t-1
        // where R(x) is the received polynomial
        for (int i = 0; i < 2 * d_t; i++) {
            uint8_t alpha_power = d_gf.power(2, i + 1); // alpha^(i+1)
            syndromes[i] = 0;
            
            // Evaluate at alpha^(i+1); coefficient order matches decode_rs Horner (byte j -> x^{n-1-j})
            for (int j = 0; j < d_n && j < (int)received.size(); j++) {
                if (received[j] != 0) {
                    const int deg = d_n - 1 - j;
                    uint8_t term =
                        d_gf.multiply(received[j], d_gf.power(alpha_power, deg));
                    syndromes[i] = d_gf.add(syndromes[i], term);
                }
            }
            
            if (syndromes[i] != 0) {
                has_errors = true;
            }
        }
        
        return has_errors;
    }

    /** Phil Karn decode_rs.h (gr-fec): NN=255, PAD=0, FCR=1, PRIM=1, IPRIM=1, no erasures. */
    bool decode_rs_inplace(std::vector<uint8_t>& data)
    {
        const unsigned NROOTS = static_cast<unsigned>(2 * d_t);
        const unsigned NN = static_cast<unsigned>(d_n);
        const unsigned A0 = NN;
        const unsigned FCR = 1;
        const unsigned PRIM = 1;
        const unsigned IPRIM = 1;
        const int no_eras = 0;

        std::vector<uint8_t> s(NROOTS);
        std::vector<uint8_t> lambda(NROOTS + 1, 0), b(NROOTS + 1), t(NROOTS + 1);
        std::vector<uint8_t> omega(NROOTS + 1, 0);
        std::vector<uint8_t> reg(NROOTS + 1, 0);
        std::vector<unsigned> root(NROOTS), loc(NROOTS);

        int deg_lambda = 0, el = 0, deg_omega = 0;
        int i, j, r;
        unsigned syn_error = 0;
        int count = 0;

        for (i = 0; (unsigned)i < NROOTS; i++)
            s[(unsigned)i] = data[0];

        for (j = 1; (unsigned)j < NN; j++) {
            for (i = 0; (unsigned)i < NROOTS; i++) {
                if (s[(unsigned)i] == 0)
                    s[(unsigned)i] = data[(unsigned)j];
                else {
                    unsigned idx = GaloisField256::gf_modnn(d_gf.gf_index_of(s[(unsigned)i]) +
                                                            (FCR + (unsigned)i) * PRIM);
                    s[(unsigned)i] = d_gf.add(data[(unsigned)j], d_gf.gf_alpha_to(idx));
                }
            }
        }

        for (i = 0; (unsigned)i < NROOTS; i++) {
            syn_error |= s[(unsigned)i];
            s[(unsigned)i] = static_cast<uint8_t>(d_gf.gf_index_of(s[(unsigned)i]));
        }

        if (!syn_error) {
            count = 0;
            goto finish;
        }

        std::fill(lambda.begin() + 1, lambda.end(), static_cast<uint8_t>(0));
        lambda[0] = 1;

        for (i = 0; (unsigned)i < NROOTS + 1u; i++)
            b[(unsigned)i] = static_cast<uint8_t>(d_gf.gf_index_of(lambda[(unsigned)i]));

        r = no_eras;
        el = no_eras;
        while ((unsigned)(++r) <= NROOTS) {
            uint8_t discr_r_poly = 0;
            for (i = 0; i < r; i++) {
                if ((lambda[(unsigned)i] != 0) && (s[(unsigned)(r - i - 1)] != static_cast<uint8_t>(A0))) {
                    unsigned idx = GaloisField256::gf_modnn(d_gf.gf_index_of(lambda[(unsigned)i]) +
                                                            s[(unsigned)(r - i - 1)]);
                    discr_r_poly = d_gf.add(discr_r_poly, d_gf.gf_alpha_to(idx));
                }
            }
            uint8_t discr_r = static_cast<uint8_t>(d_gf.gf_index_of(discr_r_poly));
            if (discr_r == static_cast<uint8_t>(A0)) {
                std::copy_backward(b.begin(), b.begin() + static_cast<std::ptrdiff_t>(NROOTS),
                                   b.begin() + static_cast<std::ptrdiff_t>(NROOTS + 1));
                b[0] = static_cast<uint8_t>(A0);
            } else {
                t[0] = lambda[0];
                for (i = 0; (unsigned)i < NROOTS; i++) {
                    if (b[(unsigned)i] != static_cast<uint8_t>(A0))
                        t[(unsigned)i + 1] =
                            d_gf.add(lambda[(unsigned)i + 1],
                                     d_gf.gf_alpha_to(GaloisField256::gf_modnn((unsigned)discr_r +
                                                                               b[(unsigned)i])));
                    else
                        t[(unsigned)i + 1] = lambda[(unsigned)i + 1];
                }
                if (2 * el <= r + no_eras - 1) {
                    el = r + no_eras - el;
                    for (i = 0; (unsigned)i <= NROOTS; i++)
                        b[(unsigned)i] = (lambda[(unsigned)i] == 0)
                                             ? static_cast<uint8_t>(A0)
                                             : static_cast<uint8_t>(GaloisField256::gf_modnn(
                                                   (unsigned)d_gf.gf_index_of(lambda[(unsigned)i]) -
                                                   (unsigned)discr_r + NN));
                } else {
                    std::copy_backward(b.begin(), b.begin() + static_cast<std::ptrdiff_t>(NROOTS),
                                       b.begin() + static_cast<std::ptrdiff_t>(NROOTS + 1));
                    b[0] = static_cast<uint8_t>(A0);
                }
                std::copy(t.begin(), t.end(), lambda.begin());
            }
        }

        deg_lambda = 0;
        for (i = 0; (unsigned)i < NROOTS + 1u; i++) {
            lambda[(unsigned)i] = static_cast<uint8_t>(d_gf.gf_index_of(lambda[(unsigned)i]));
            if (lambda[(unsigned)i] != static_cast<uint8_t>(A0))
                deg_lambda = i;
        }

        std::copy(lambda.begin() + 1, lambda.begin() + 1 + static_cast<std::ptrdiff_t>(NROOTS),
                  reg.begin() + 1);
        count = 0;
        {
            unsigned ii, loc_k;
            for (ii = 1, loc_k = IPRIM - 1; ii <= NN; ii++, loc_k = GaloisField256::gf_modnn(loc_k + IPRIM)) {
                uint8_t q = 1;
                for (j = deg_lambda; j > 0; j--) {
                    if (reg[(unsigned)j] != static_cast<uint8_t>(A0)) {
                        reg[(unsigned)j] =
                            static_cast<uint8_t>(GaloisField256::gf_modnn(reg[(unsigned)j] + (unsigned)j));
                        q = d_gf.add(q, d_gf.gf_alpha_to(reg[(unsigned)j]));
                    }
                }
                if (q != 0)
                    continue;
                root[(unsigned)count] = ii;
                loc[(unsigned)count] = loc_k;
                if (++count == deg_lambda)
                    break;
            }
        }
        if (deg_lambda != count) {
            count = -1;
            goto finish;
        }

        deg_omega = 0;
        for (i = 0; (unsigned)i < NROOTS; i++) {
            uint8_t tmp = 0;
            j = (deg_lambda < i) ? deg_lambda : static_cast<int>(i);
            for (; j >= 0; j--) {
                if ((s[(unsigned)(i - j)] != static_cast<uint8_t>(A0)) &&
                    (lambda[(unsigned)j] != static_cast<uint8_t>(A0))) {
                    unsigned idx =
                        GaloisField256::gf_modnn(s[(unsigned)(i - j)] + lambda[(unsigned)j]);
                    tmp = d_gf.add(tmp, d_gf.gf_alpha_to(idx));
                }
            }
            if (tmp != 0)
                deg_omega = i;
            omega[(unsigned)i] = static_cast<uint8_t>(d_gf.gf_index_of(tmp));
        }
        omega[NROOTS] = static_cast<uint8_t>(A0);

        for (j = count - 1; j >= 0; j--) {
            uint8_t num1 = 0;
            for (i = deg_omega; i >= 0; i--) {
                if (omega[(unsigned)i] != static_cast<uint8_t>(A0)) {
                    unsigned idx = GaloisField256::gf_modnn((unsigned)omega[(unsigned)i] +
                                                            (unsigned)i * root[(unsigned)j]);
                    num1 = d_gf.add(num1, d_gf.gf_alpha_to(idx));
                }
            }
            uint8_t num2 =
                d_gf.gf_alpha_to(GaloisField256::gf_modnn(root[(unsigned)j] * (FCR - 1u) + NN));
            uint8_t den = 0;
            unsigned lim = std::min(static_cast<unsigned>(deg_lambda), NROOTS - 1u) & ~1u;
            for (i = static_cast<int>(lim); i >= 0; i -= 2) {
                if (lambda[(unsigned)(i + 1)] != static_cast<uint8_t>(A0)) {
                    unsigned idx = GaloisField256::gf_modnn(lambda[(unsigned)(i + 1)] +
                                                            (unsigned)i * root[(unsigned)j]);
                    den = d_gf.add(den, d_gf.gf_alpha_to(idx));
                }
            }
            if (den == 0) {
                count = -1;
                goto finish;
            }
            if (num1 != 0) {
                unsigned fix = GaloisField256::gf_modnn(
                    d_gf.gf_index_of(num1) + d_gf.gf_index_of(num2) + NN - d_gf.gf_index_of(den));
                data[loc[(unsigned)j]] = d_gf.add(data[loc[(unsigned)j]], d_gf.gf_alpha_to(fix));
            }
        }

    finish:
        return count >= 0;
    }

  public:
    ReedSolomonDecoder(int n, int k)
        : d_n(n != 255 ? 255 : n), d_k(k), d_t(((n != 255 ? 255 : n) - k) / 2) {}

    ~ReedSolomonDecoder() = default;

    bool decode(const std::vector<uint8_t>& in, std::vector<uint8_t>& out) {
        out.clear();
        std::vector<uint8_t> work = in;
        if ((int)work.size() < d_n)
            work.resize(d_n, 0);
        else if ((int)work.size() > d_n)
            work.resize(d_n);

        std::vector<uint8_t> syndromes;
        if (!calculate_syndromes(work, syndromes)) {
            out.assign(work.begin(), work.begin() + d_k);
            return true;
        }
        if (!decode_rs_inplace(work))
            return false;
        std::vector<uint8_t> syn_check;
        if (calculate_syndromes(work, syn_check)) {
            out.clear();
            return false;
        }
        out.assign(work.begin(), work.begin() + d_k);
        return true;
    }

    std::vector<uint8_t> decode(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> out;
        if (!decode(data, out))
            return {};
        return out;
    }

    int get_code_length() const {
        return d_n;
    }
    int get_data_length() const {
        return d_k;
    }
    int get_error_correction_capability() const {
        return d_t;
    }
};

#endif /* INCLUDED_PACKET_PROTOCOLS_COMMON_H */

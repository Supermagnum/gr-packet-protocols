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
    uint8_t d_alpha_to[GF_SIZE];
    uint8_t d_index_of[GF_SIZE];
    bool d_initialized;

    void initialize() {
        if (d_initialized) return;
        
        // Build log and antilog tables for GF(256)
        // Field elements: 0, 1, alpha, alpha^2, ..., alpha^254
        d_index_of[0] = 0xFF; // Invalid (log of 0 is undefined)
        d_alpha_to[255] = 0;  // alpha^255 = 1 (wrap around)
        
        uint8_t sr = 1; // Start with alpha^0 = 1
        for (int i = 0; i < 255; i++) {
            d_alpha_to[i] = sr;      // alpha^i = sr
            d_index_of[sr] = i;       // log(sr) = i
            sr <<= 1;                 // Multiply by alpha (shift left)
            if (sr & 0x100) {         // If overflow (bit 8 set)
                sr ^= PRIMITIVE_POLY; // Reduce modulo primitive polynomial
            }
            sr &= 0xFF;               // Keep only 8 bits
        }
        
        d_initialized = true;
    }

  public:
    GaloisField256() : d_initialized(false) {
        initialize();
    }

    uint8_t multiply(uint8_t a, uint8_t b) {
        if (a == 0 || b == 0) return 0;
        int sum = d_index_of[a] + d_index_of[b];
        if (sum >= 255) sum -= 255;
        return d_alpha_to[sum];
    }

    uint8_t divide(uint8_t a, uint8_t b) {
        if (a == 0) return 0;
        if (b == 0) return 0; // Division by zero
        int diff = d_index_of[a] - d_index_of[b];
        if (diff < 0) diff += 255;
        return d_alpha_to[diff];
    }

    uint8_t power(uint8_t a, int n) {
        if (a == 0) return 0;
        if (n == 0) return 1;
        int exp = (d_index_of[a] * n) % 255;
        return d_alpha_to[exp];
    }

    uint8_t add(uint8_t a, uint8_t b) {
        return a ^ b; // Addition in GF(2^8) is XOR
    }

    uint8_t subtract(uint8_t a, uint8_t b) {
        return a ^ b; // Subtraction in GF(2^8) is same as addition
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

    void build_generator_poly() {
        // Generator polynomial: g(x) = (x - alpha^1)(x - alpha^2)...(x - alpha^(2t))
        // Note: Standard RS codes use alpha^1 through alpha^2t (not alpha^0)
        d_generator_poly.resize(2 * d_t + 1, 0);
        d_generator_poly[0] = 1; // g(x) = 1 initially
        
        // Multiply by (x - alpha^i) for i = 1 to 2t
        for (int i = 1; i <= 2 * d_t; i++) {
            uint8_t alpha_i = d_gf.power(2, i); // alpha^i
            
            // Multiply current g(x) by (x - alpha^i)
            // This is: g(x) * (x - alpha^i) = g(x) * x - g(x) * alpha^i
            // Which means: shift g(x) left, then subtract alpha^i * g(x)
            
            // Save current polynomial
            std::vector<uint8_t> temp = d_generator_poly;
            
            // Shift left (multiply by x)
            for (int j = 2 * d_t; j >= 1; j--) {
                d_generator_poly[j] = d_generator_poly[j - 1];
            }
            d_generator_poly[0] = 0;
            
            // Subtract alpha^i * temp (which is adding in GF(2^8))
            for (int j = 0; j < 2 * d_t; j++) {
                d_generator_poly[j] = d_gf.add(d_generator_poly[j],
                                               d_gf.multiply(temp[j], alpha_i));
            }
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
        
        // Copy data to result (high-order coefficients)
        int data_len = (data.size() < (size_t)d_k) ? data.size() : d_k;
        for (int i = 0; i < data_len; i++) {
            result[i] = data[i];
        }
        // Zero pad if needed
        for (int i = data_len; i < d_k; i++) {
            result[i] = 0;
        }
        
        // Calculate parity symbols using polynomial division
        // Divide message polynomial x^(2t) * m(x) by generator polynomial g(x)
        // The remainder is the parity polynomial
        
        // Create message polynomial multiplied by x^(2t)
        std::vector<uint8_t> msg_poly(d_k + 2 * d_t, 0);
        for (int i = 0; i < d_k; i++) {
            msg_poly[i] = result[i];
        }
        
        // Polynomial division: divide msg_poly by generator_poly
        for (int i = 0; i < d_k; i++) {
            uint8_t coef = msg_poly[i];
            if (coef != 0) {
                for (int j = 0; j <= 2 * d_t; j++) {
                    if (d_generator_poly[j] != 0) {
                        msg_poly[i + j] = d_gf.add(msg_poly[i + j],
                                                   d_gf.multiply(coef, d_generator_poly[j]));
                    }
                }
            }
        }
        
        // Copy parity symbols (remainder) to result
        for (int i = 0; i < 2 * d_t; i++) {
            result[d_k + i] = msg_poly[d_k + i];
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
            
            // Evaluate received polynomial at alpha^(i+1)
            // R(alpha) = r_0 + r_1*alpha + r_2*alpha^2 + ... + r_(n-1)*alpha^(n-1)
            for (int j = 0; j < d_n && j < (int)received.size(); j++) {
                if (received[j] != 0) {
                    uint8_t term = d_gf.multiply(received[j], d_gf.power(alpha_power, j));
                    syndromes[i] = d_gf.add(syndromes[i], term);
                }
            }
            
            if (syndromes[i] != 0) {
                has_errors = true;
            }
        }
        
        return has_errors;
    }

    // Berlekamp-Massey algorithm to find error locator polynomial
    int berlekamp_massey(const std::vector<uint8_t>& syndromes, std::vector<uint8_t>& lambda) {
        std::vector<uint8_t> C(2 * d_t + 1, 0);
        std::vector<uint8_t> B(2 * d_t + 1, 0);
        C[0] = 1;
        B[0] = 1;
        int L = 0;
        int m = 1;
        uint8_t b = 1;
        
        for (int n = 0; n < 2 * d_t; n++) {
            uint8_t d = syndromes[n];
            for (int i = 1; i <= L; i++) {
                d = d_gf.add(d, d_gf.multiply(C[i], syndromes[n - i]));
            }
            
            if (d == 0) {
                m++;
            } else {
                std::vector<uint8_t> T = C;
                uint8_t inv_b = d_gf.divide(1, b);
                
                for (int i = 0; i <= 2 * d_t - m; i++) {
                    if (B[i] != 0) {
                        C[i + m] = d_gf.add(C[i + m], d_gf.multiply(d, d_gf.multiply(inv_b, B[i])));
                    }
                }
                
                if (2 * L <= n) {
                    L = n + 1 - L;
                    B = T;
                    b = d;
                    m = 1;
                } else {
                    m++;
                }
            }
        }
        
        lambda = C;
        return L;
    }

    // Chien search to find error positions
    void chien_search(const std::vector<uint8_t>& lambda, int L, std::vector<int>& error_positions) {
        error_positions.clear();
        
        // Evaluate Lambda(x) at alpha^-i for i = 0 to n-1
        // Error position is n-1-i where Lambda(alpha^-i) = 0
        for (int i = 0; i < d_n; i++) {
            uint8_t alpha_inv_i = d_gf.power(2, 255 - i); // alpha^-i
            uint8_t sum = lambda[0];
            
            for (int j = 1; j <= L && j < (int)lambda.size(); j++) {
                if (lambda[j] != 0) {
                    sum = d_gf.add(sum, d_gf.multiply(lambda[j], d_gf.power(alpha_inv_i, j)));
                }
            }
            
            if (sum == 0) {
                error_positions.push_back(d_n - 1 - i);
            }
        }
    }

    // Forney algorithm to find error values
    void forney_algorithm(const std::vector<uint8_t>& syndromes, 
                         const std::vector<uint8_t>& lambda,
                         const std::vector<int>& error_positions,
                         std::vector<uint8_t>& error_values) {
        error_values.resize(error_positions.size());
        
        // Build error evaluator polynomial Omega(x)
        // Omega(x) = S(x) * Lambda(x) mod x^(2t)
        std::vector<uint8_t> omega(2 * d_t, 0);
        for (int i = 0; i < 2 * d_t; i++) {
            omega[i] = syndromes[i];
            for (int j = 1; j <= i && j < (int)lambda.size(); j++) {
                omega[i] = d_gf.add(omega[i], d_gf.multiply(lambda[j], syndromes[i - j]));
            }
        }
        
        for (size_t i = 0; i < error_positions.size(); i++) {
            int pos = error_positions[i];
            uint8_t alpha_inv = d_gf.power(2, 255 - (d_n - 1 - pos)); // alpha^-(n-1-pos)
            
            // Calculate numerator: Omega(alpha^-pos)
            uint8_t numerator = 0;
            for (int j = 0; j < 2 * d_t; j++) {
                if (omega[j] != 0) {
                    numerator = d_gf.add(numerator,
                                        d_gf.multiply(omega[j], d_gf.power(alpha_inv, j)));
                }
            }
            
            // Calculate denominator: Lambda'(alpha^-pos)
            // Lambda'(x) is the formal derivative
            // In GF(2^m), derivative of x^j is: 0 if j is even, x^(j-1) if j is odd
            uint8_t denominator = 0;
            for (int j = 1; j < (int)lambda.size(); j++) {
                if (lambda[j] != 0 && (j % 2 == 1)) {
                    // Only odd powers contribute to derivative
                    uint8_t coef = lambda[j];
                    denominator = d_gf.add(denominator,
                                          d_gf.multiply(coef, d_gf.power(alpha_inv, j - 1)));
                }
            }
            
            if (denominator != 0) {
                error_values[i] = d_gf.divide(numerator, denominator);
            } else {
                error_values[i] = 0; // Cannot determine error value
            }
        }
    }

  public:
    ReedSolomonDecoder(int n, int k) : d_n(n), d_k(k), d_t((n - k) / 2) {
        if (n != 255) {
            // Only support RS(255,k) codes
            d_n = 255;
            d_k = k;
            d_t = (d_n - d_k) / 2;
        }
    }
    
    ~ReedSolomonDecoder() = default;

    std::vector<uint8_t> decode(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> result = data;
        
        // Ensure we have full codeword
        if ((int)result.size() < d_n) {
            result.resize(d_n, 0);
        }
        
        // Calculate syndromes
        std::vector<uint8_t> syndromes;
        bool has_errors = calculate_syndromes(result, syndromes);
        
        if (!has_errors) {
            // No errors, return data portion
            result.resize(d_k);
            return result;
        }
        
        // Find error locator polynomial using Berlekamp-Massey
        std::vector<uint8_t> lambda(2 * d_t + 1, 0);
        int L = berlekamp_massey(syndromes, lambda);
        
        if (L < 0 || L > d_t) {
            // Too many errors, cannot correct
            result.resize(d_k);
            return result;
        }
        
        // Find error positions using Chien search
        std::vector<int> error_positions;
        chien_search(lambda, L, error_positions);
        
        if ((int)error_positions.size() != L) {
            // Error count mismatch, cannot correct
            result.resize(d_k);
            return result;
        }
        
        // Find error values using Forney algorithm
        std::vector<uint8_t> error_values;
        forney_algorithm(syndromes, lambda, error_positions, error_values);
        
        // Correct errors
        for (size_t i = 0; i < error_positions.size(); i++) {
            int pos = error_positions[i];
            if (pos >= 0 && pos < d_n) {
                result[pos] = d_gf.add(result[pos], error_values[i]);
            }
        }
        
        // Return data portion
        result.resize(d_k);
        return result;
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

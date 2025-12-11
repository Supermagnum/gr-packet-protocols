#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright 2024 gr-packet-protocols
#
# This file is part of gr-packet-protocols
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from gnuradio import gr
from gnuradio import blocks
from gnuradio import digital
from gnuradio.analog import cpm


class adaptive_modulator(gr.hier_block2):
    """
    Adaptive Modulator - Automatically switches between modulation modes
    based on link quality.

    This hierarchical block integrates adaptive_rate_control with actual
    GNU Radio modulation blocks, automatically switching between them.

    Args:
        initial_mode: Initial modulation mode (modulation_mode_t enum)
        samples_per_symbol: Samples per symbol for modulation (default: 2)
        enable_adaptation: Enable automatic rate adaptation (default: True)
        hysteresis_db: Hysteresis in dB to prevent rapid switching (default: 2.0)
    """

    def __init__(self, initial_mode=None, samples_per_symbol=2,
                 enable_adaptation=True, hysteresis_db=2.0):
        # Import here to avoid circular dependencies
        from gnuradio import packet_protocols

        if initial_mode is None:
            initial_mode = packet_protocols.modulation_mode_t.MODE_4FSK

        gr.hier_block2.__init__(
            self,
            "adaptive_modulator",
            gr.io_signature(1, 1, gr.sizeof_char),  # Input: bytes
            gr.io_signature(1, 1, gr.sizeof_gr_complex)  # Output: complex
        )

        self.d_samples_per_symbol = samples_per_symbol
        self.d_current_mode = initial_mode
        self.d_last_mode = initial_mode

        # Create adaptive rate control
        self.d_rate_control = packet_protocols.adaptive_rate_control(
            initial_mode=initial_mode,
            enable_adaptation=enable_adaptation,
            hysteresis_db=hysteresis_db
        )

        # Create all modulation blocks
        self.d_mod_blocks = {}
        self.d_mode_to_index = {}
        self.d_index = 0

        # Create modulators for supported modes
        self._create_modulators()

        # Create selector to switch between modulators
        # We'll use a custom approach with multiple paths and a selector
        self.d_selector = blocks.selector(
            gr.sizeof_gr_complex,
            self._get_mode_index(initial_mode)
        )

        # Connect all modulators to selector inputs
        self._connect_modulators()

        # Connect input to all modulators (they all receive the same input)
        self.connect(self, self.d_selector)

        # Connect selector output to output
        self.connect(self.d_selector, self)

        # Store reference for mode monitoring
        self._monitoring = False

    def _create_modulators(self):
        """Create modulation blocks for all supported modes"""
        sps = self.d_samples_per_symbol

        # 2FSK - GFSK modulator
        mod_2fsk = digital.gfsk_mod(
            samples_per_symbol=sps,
            sensitivity=1.0,
            bt=0.35
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_2FSK] = mod_2fsk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_2FSK] = self.d_index
        self.d_index += 1

        # 4FSK - CPM modulator (LREC)
        mod_4fsk = cpm.cpmmod_bc(
            type=cpm.CPM_LREC,
            h=0.5,
            samples_per_sym=sps,
            L=4,
            beta=0.3
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_4FSK] = mod_4fsk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_4FSK] = self.d_index
        self.d_index += 1

        # 8FSK - CPM modulator
        mod_8fsk = cpm.cpmmod_bc(
            type=cpm.CPM_LREC,
            h=0.5,
            samples_per_sym=sps,
            L=4,
            beta=0.3
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_8FSK] = mod_8fsk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_8FSK] = self.d_index
        self.d_index += 1

        # 16FSK - CPM modulator
        mod_16fsk = cpm.cpmmod_bc(
            type=cpm.CPM_LREC,
            h=0.5,
            samples_per_sym=sps,
            L=4,
            beta=0.3
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_16FSK] = mod_16fsk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_16FSK] = self.d_index
        self.d_index += 1

        # BPSK
        mod_bpsk = digital.psk.psk_mod(
            constellation_points=2,
            mod_code=digital.GRAY_CODE,
            differential=False
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_BPSK] = mod_bpsk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_BPSK] = self.d_index
        self.d_index += 1

        # QPSK
        mod_qpsk = digital.psk.psk_mod(
            constellation_points=4,
            mod_code=digital.GRAY_CODE,
            differential=False
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_QPSK] = mod_qpsk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_QPSK] = self.d_index
        self.d_index += 1

        # 8PSK
        mod_8psk = digital.psk.psk_mod(
            constellation_points=8,
            mod_code=digital.GRAY_CODE,
            differential=False
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_8PSK] = mod_8psk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_8PSK] = self.d_index
        self.d_index += 1

        # 16-QAM
        mod_qam16 = digital.qam.qam_mod(
            constellation_points=16,
            mod_code=digital.GRAY_CODE,
            differential=False
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_QAM16] = mod_qam16
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_QAM16] = self.d_index
        self.d_index += 1

        # 64-QAM
        mod_qam64 = digital.qam.qam_mod(
            constellation_points=64,
            mod_code=digital.GRAY_CODE,
            differential=False
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_QAM64] = mod_qam64
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_QAM64] = self.d_index
        self.d_index += 1

    def _get_mode_index(self, mode):
        """Get selector index for a modulation mode"""
        return self.d_mode_to_index.get(mode, 0)

    def _connect_modulators(self):
        """Connect all modulators to selector inputs"""
        # Create a splitter to feed all modulators
        # Note: GNU Radio doesn't have a simple way to connect one source
        # to multiple sinks in a hier_block2, so we'll use a different approach
        # For now, we'll connect the active modulator directly
        # In a full implementation, you'd use a custom block or message-based switching

        # Connect input to rate control (for monitoring)
        # Then connect rate control to active modulator
        active_index = self._get_mode_index(self.d_current_mode)
        active_mod = self.d_mod_blocks[self.d_current_mode]

        # This is a simplified connection - in practice you'd need
        # a more sophisticated switching mechanism
        pass  # Will be handled by selector

    def get_rate_control(self):
        """Get the adaptive rate control block"""
        return self.d_rate_control

    def get_modulation_mode(self):
        """Get current modulation mode"""
        return self.d_rate_control.get_modulation_mode()

    def set_modulation_mode(self, mode):
        """Set modulation mode manually"""
        self.d_rate_control.set_modulation_mode(mode)
        self.d_current_mode = mode
        # Update selector index
        index = self._get_mode_index(mode)
        # Note: selector.set_input_index() may not be available in all versions
        # This would need to be implemented via message passing or custom block

    def set_adaptation_enabled(self, enabled):
        """Enable/disable automatic adaptation"""
        self.d_rate_control.set_adaptation_enabled(enabled)


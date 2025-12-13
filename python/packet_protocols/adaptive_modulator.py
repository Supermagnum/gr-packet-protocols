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

# Import modulation_switch - try installed module first, fall back to local
try:
    from gnuradio.packet_protocols import modulation_switch
except ImportError:
    # Fall back to local import if installed module doesn't have it
    try:
        from .modulation_switch import modulation_switch
    except ImportError:
        # Last resort: import from same directory
        from modulation_switch import modulation_switch


class adaptive_modulator(gr.hier_block2):
    """
    Adaptive Modulator - Automatically switches between modulation modes
    based on link quality.

    This hierarchical block integrates adaptive_rate_control with actual
    GNU Radio modulation blocks, automatically switching between them.

    IMPORTANT: Baud Rate vs. Bit Rate
    ---------------------------------
    The actual baud rate (symbol rate) depends on the sample rate of your
    flowgraph, not just samples_per_symbol. The relationship is:
    
        baud_rate = sample_rate / samples_per_symbol
    
    For example, with sample_rate=96000 and samples_per_symbol=2:
        baud_rate = 96000 / 2 = 48000 baud
    
    The bit rate (bps) depends on the modulation scheme:
        - 2FSK: 1 bit/symbol → 48000 bps
        - 4FSK: 2 bits/symbol → 96000 bps
        - QPSK: 2 bits/symbol → 96000 bps
        - 16-QAM: 4 bits/symbol → 192000 bps
        - 64-QAM: 6 bits/symbol → 288000 bps
        - 256-QAM: 8 bits/symbol → 384000 bps
    
    To achieve specific baud rates (e.g., 1200, 2400, 6250, 12500), you must
    configure your flowgraph's sample rate accordingly. For example:
        - 1200 baud: sample_rate = 1200 * samples_per_symbol = 2400 Hz (with sps=2)
        - 2400 baud: sample_rate = 2400 * samples_per_symbol = 4800 Hz (with sps=2)
        - 6250 baud: sample_rate = 6250 * samples_per_symbol = 12500 Hz (with sps=2)
        - 12500 baud: sample_rate = 12500 * samples_per_symbol = 25000 Hz (with sps=2)

    Args:
        initial_mode: Initial modulation mode (modulation_mode_t enum)
        samples_per_symbol: Samples per symbol for modulation (default: 2)
        enable_adaptation: Enable automatic rate adaptation (default: True)
        hysteresis_db: Hysteresis in dB to prevent rapid switching (default: 2.0)
    
    Note:
        All modulators process the input stream simultaneously. The switch
        block selects which modulator's output to use. This means the input
        data is consumed by all modulators, which is necessary for seamless
        mode switching but may affect performance with many modulation modes.
    """

    def __init__(self, initial_mode=None, samples_per_symbol=2,
                 enable_adaptation=True, hysteresis_db=2.0):
        # Import here to avoid circular dependencies
        # Try installed module first, handle gracefully if incomplete
        try:
            from gnuradio import packet_protocols
        except (ImportError, ModuleNotFoundError):
            # If installed module fails (e.g., missing pluto_ptt_control),
            # try to import just what we need from C++ bindings
            try:
                # Import the C++ bindings directly
                from . import packet_protocols_python as pp_python
                # Create a minimal packet_protocols module with needed attributes
                class PacketProtocols:
                    modulation_mode_t = pp_python.modulation_mode_t
                    adaptive_rate_control = pp_python.adaptive_rate_control
                packet_protocols = PacketProtocols()
            except (ImportError, AttributeError) as e:
                # If C++ bindings also fail, we can't proceed
                raise ImportError(
                    f"Cannot import packet_protocols module. "
                    f"Both installed module and C++ bindings failed. "
                    f"Please rebuild and reinstall the module. "
                    f"Original error: {e}"
                )

        if initial_mode is None:
            initial_mode = packet_protocols.modulation_mode_t.MODE_2FSK

        gr.hier_block2.__init__(
            self,
            "adaptive_modulator",
            gr.io_signature(1, 1, gr.sizeof_char),  # Input: bytes
            gr.io_signature(1, 1, gr.sizeof_gr_complex)  # Output: complex
        )

        # Store packet_protocols for use in _create_modulators
        self.d_packet_protocols = packet_protocols

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

        # Create modulation switch block to select between modulator outputs
        num_mods = len(self.d_mod_blocks)
        self.d_switch = modulation_switch(
            self.d_rate_control,
            self.d_mode_to_index,
            num_mods
        )

        # Connect input to all modulators and modulators to switch
        # In GNU Radio, you can connect one source to multiple sinks
        # All modulators will process the same input, and the switch selects the output
        self._connect_modulators()

        # Connect switch output to hier_block2 output
        self.connect(self.d_switch, self)

        # Store reference for mode monitoring
        self._monitoring = False

    def _create_modulators(self):
        """Create modulation blocks for all supported modes"""
        sps = self.d_samples_per_symbol
        packet_protocols = self.d_packet_protocols

        # 2FSK - GFSK modulator
        mod_2fsk = digital.gfsk_mod(
            samples_per_symbol=sps,
            sensitivity=1.0,
            bt=0.35
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_2FSK] = mod_2fsk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_2FSK] = self.d_index
        self.d_index += 1

        # 4FSK - Use qradiolink.mod_4fsk (proper 4FSK implementation)
        from gnuradio import qradiolink
        mod_4fsk = qradiolink.mod_4fsk(
            sps=sps,
            samp_rate=sps * 48000,  # Sample rate based on sps
            carrier_freq=0,
            filter_width=5000,
            fm=True
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_4FSK] = mod_4fsk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_4FSK] = self.d_index
        self.d_index += 1

        # 8FSK - Use qradiolink.mod_4fsk with different configuration
        # Note: qradiolink doesn't have mod_8fsk, so we'll use mod_4fsk as approximation
        # For true 8FSK, would need custom implementation
        mod_8fsk = qradiolink.mod_4fsk(
            sps=sps,
            samp_rate=sps * 48000,
            carrier_freq=0,
            filter_width=5000,
            fm=True
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_8FSK] = mod_8fsk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_8FSK] = self.d_index
        self.d_index += 1

        # 16FSK - Use qradiolink.mod_4fsk with different configuration
        # Note: qradiolink doesn't have mod_16fsk, so we'll use mod_4fsk as approximation
        # For true 16FSK, would need custom implementation
        mod_16fsk = qradiolink.mod_4fsk(
            sps=sps,
            samp_rate=sps * 48000,
            carrier_freq=0,
            filter_width=5000,
            fm=True
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_16FSK] = mod_16fsk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_16FSK] = self.d_index
        self.d_index += 1

        # BPSK
        mod_bpsk = digital.psk.psk_mod(
            constellation_points=2,
            mod_code='gray',
            differential=False
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_BPSK] = mod_bpsk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_BPSK] = self.d_index
        self.d_index += 1

        # QPSK
        mod_qpsk = digital.psk.psk_mod(
            constellation_points=4,
            mod_code='gray',
            differential=False
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_QPSK] = mod_qpsk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_QPSK] = self.d_index
        self.d_index += 1

        # 8PSK
        mod_8psk = digital.psk.psk_mod(
            constellation_points=8,
            mod_code='gray',
            differential=False
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_8PSK] = mod_8psk
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_8PSK] = self.d_index
        self.d_index += 1

        # 16-QAM
        qam16_const = digital.qam.qam_constellation(
            constellation_points=16,
            differential=False,
            mod_code='gray'
        )
        mod_qam16 = digital.generic_mod(
            constellation=qam16_const,
            differential=False,
            samples_per_symbol=sps
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_QAM16] = mod_qam16
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_QAM16] = self.d_index
        self.d_index += 1

        # 64-QAM @ 6,250 baud (37,500 bps)
        qam64_const = digital.qam.qam_constellation(
            constellation_points=64,
            differential=False,
            mod_code='gray'
        )
        mod_qam64_6250 = digital.generic_mod(
            constellation=qam64_const,
            differential=False,
            samples_per_symbol=sps
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_QAM64_6250] = mod_qam64_6250
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_QAM64_6250] = self.d_index
        self.d_index += 1

        # 64-QAM @ 12,500 baud (75,000 bps)
        # Same constellation, different baud rate (handled by sample rate in flowgraph)
        mod_qam64_12500 = digital.generic_mod(
            constellation=qam64_const,
            differential=False,
            samples_per_symbol=sps
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_QAM64_12500] = mod_qam64_12500
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_QAM64_12500] = self.d_index
        self.d_index += 1

        # 256-QAM @ 12,500 baud (100,000 bps)
        qam256_const = digital.qam.qam_constellation(
            constellation_points=256,
            differential=False,
            mod_code='gray'
        )
        mod_qam256 = digital.generic_mod(
            constellation=qam256_const,
            differential=False,
            samples_per_symbol=sps
        )
        self.d_mod_blocks[packet_protocols.modulation_mode_t.MODE_QAM256] = mod_qam256
        self.d_mode_to_index[packet_protocols.modulation_mode_t.MODE_QAM256] = self.d_index
        self.d_index += 1

    def _get_mode_index(self, mode):
        """Get selector index for a modulation mode"""
        return self.d_mode_to_index.get(mode, 0)

    def _connect_modulators(self):
        """
        Connect input to all modulators and modulators to switch inputs.
        
        In GNU Radio, connecting one source to multiple sinks is allowed.
        All modulators will process the same input data, and the switch
        block selects which modulator's output to use based on the current mode.
        """
        # Create a list of modulators ordered by their index
        # The switch block expects inputs in index order (0, 1, 2, ...)
        mods_by_index = [None] * len(self.d_mod_blocks)
        for mode, mod_block in self.d_mod_blocks.items():
            idx = self._get_mode_index(mode)
            if 0 <= idx < len(mods_by_index):
                mods_by_index[idx] = (mode, mod_block)
        
        # Connect hier_block2 input to each modulator
        # All modulators receive the same input stream
        for idx, (mode, mod_block) in enumerate(mods_by_index):
            if mod_block is not None:
                self.connect(self, mod_block)
        
        # Connect each modulator output to the corresponding switch input
        # The switch block has multiple inputs, one for each modulator
        for idx, (mode, mod_block) in enumerate(mods_by_index):
            if mod_block is not None:
                # Connect modulator output to switch input at index idx
                self.connect(mod_block, (self.d_switch, idx))

    def get_rate_control(self):
        """Get the adaptive rate control block"""
        return self.d_rate_control

    def get_modulation_mode(self):
        """Get current modulation mode"""
        return self.d_rate_control.get_modulation_mode()

    def set_modulation_mode(self, mode):
        """
        Set modulation mode manually.
        
        The modulation_switch block will automatically detect the mode change
        via its work() method which polls the rate_control for the current mode.
        """
        self.d_rate_control.set_modulation_mode(mode)
        self.d_current_mode = mode
        # The modulation_switch block will pick up the mode change automatically
        # in its work() method, so no explicit update needed here

    def set_adaptation_enabled(self, enabled):
        """Enable/disable automatic adaptation"""
        self.d_rate_control.set_adaptation_enabled(enabled)


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
import pmt
import numpy


class modulation_switch(gr.sync_block):
    """
    Custom block that switches between multiple modulation inputs based on
    adaptive_rate_control mode selection.

    This block takes multiple complex inputs (one per modulation mode) and
    outputs the signal from the currently selected mode.

    Args:
        rate_control: adaptive_rate_control block instance
        mode_to_index: Dictionary mapping modulation_mode_t to input index
        num_inputs: Number of input ports (number of modulation modes)
    """
    def __init__(self, rate_control, mode_to_index, num_inputs=4):
        # For multiple inputs, we need to specify the type for each input
        # Use numpy dtype instead of sizeof
        gr.sync_block.__init__(
            self,
            name="modulation_switch",
            in_sig=[numpy.complex64] * num_inputs,
            out_sig=[numpy.complex64]
        )
        self.rate_control = rate_control
        self.mode_to_index = mode_to_index
        self.current_index = 0
        self.last_mode = None

        # Register message port for mode change notifications
        self.message_port_register_in(pmt.intern("mode_update"))
        self.set_msg_handler(pmt.intern("mode_update"), self.handle_mode_update)

    def handle_mode_update(self, msg):
        """Handle mode update message"""
        try:
            current_mode = self.rate_control.get_modulation_mode()
            if current_mode != self.last_mode and current_mode in self.mode_to_index:
                self.current_index = self.mode_to_index[current_mode]
                self.last_mode = current_mode
        except Exception as e:
            print(f"Error in mode update: {e}")

    def work(self, input_items, output_items):
        """Switch between inputs based on current mode"""
        # Get current mode
        try:
            current_mode = self.rate_control.get_modulation_mode()
            if current_mode in self.mode_to_index:
                self.current_index = self.mode_to_index[current_mode]
        except:
            pass

        # Copy from selected input to output
        n = min(len(input_items[self.current_index]), len(output_items[0]))
        if n > 0:
            output_items[0][:n] = input_items[self.current_index][:n]
        return n


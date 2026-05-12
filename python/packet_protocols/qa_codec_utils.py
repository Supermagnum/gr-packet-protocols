# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Shared helpers for QA tests (not installed as runtime API)."""


def ax25_ui_payload(pdu: bytes) -> bytes:
    """Extract UI information field from an unpacked AX.25 PDU (dest+src+ctl+pid+info+FCS)."""
    if len(pdu) < 18:
        return b""
    return pdu[16:-2]


def fx25_first_payload_byte(decoded_block: bytes) -> int:
    """FX.25 decoder emits one full RS data block per frame; payload is zero-padded to k."""
    if not decoded_block:
        return -1
    return decoded_block[0] & 0xFF


def split_fixed_chunks(buf: bytes, chunk_len: int):
    if chunk_len <= 0 or len(buf) % chunk_len != 0:
        return []
    return [buf[i : i + chunk_len] for i in range(0, len(buf), chunk_len)]

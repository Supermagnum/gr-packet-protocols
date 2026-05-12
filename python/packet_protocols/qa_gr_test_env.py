# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""CTest helpers: resolve ``gnuradio.packet_protocols`` from this build."""

from __future__ import annotations

import os


def ensure_build_packet_protocols_first() -> None:
    """Prefer ``test_modules/gnuradio`` ahead of site-packages on ``gnuradio.__path__``.

    GNU Radio uses ``pkgutil.extend_path``. If ``gnuradio.packet_protocols`` is installed
    system-wide, it wins unless the build tree's ``gnuradio`` directory is searched first.
    CTest wrappers prepend ``PROJECT_BINARY_DIR/test_modules`` to ``PYTHONPATH``; we move
    that corresponding ``gnuradio`` package path to the front of ``gnuradio.__path__``
    before importing submodules.
    """
    pypath = os.environ.get("PYTHONPATH", "")
    if not pypath:
        return
    head = pypath.split(os.pathsep)[0]
    if not head:
        return
    candidate = os.path.join(head, "gnuradio")
    init_pp = os.path.join(candidate, "packet_protocols", "__init__.py")
    if not os.path.isfile(init_pp):
        return
    import gnuradio

    try:
        gnuradio.__path__.remove(candidate)
    except ValueError:
        pass
    gnuradio.__path__.insert(0, candidate)

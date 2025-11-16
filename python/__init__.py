# Copyright 2024 gr-packet-protocols
#
# This file is part of gr-packet-protocols
#
# gr-packet-protocols is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# gr-packet-protocols is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with gr-packet-protocols; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

# The presence of this file turns this directory into a Python package

'''
gr-packet-protocols: GNU Radio packet protocols module

This module provides implementations of:
- AX.25 protocol with KISS TNC interface
- FX.25 protocol with Forward Error Correction
- IL2P (Improved Layer 2 Protocol) with Reed-Solomon FEC
'''

# import pybind11 generated symbols into the packet_protocols namespace
import os
import importlib.util
import glob

# Try to import the compiled module
# The .so file name varies by Python version and platform
_loaded = False

# First, try the standard import
try:
    from .packet_protocols_python import *
    _loaded = True
except ImportError:
    pass

# If that failed, try to find and load the .so file directly
# This handles cases where the module is in a different location
if not _loaded:
    try:
        # Get the directory of this __init__.py file
        _current_dir = os.path.dirname(os.path.abspath(__file__))
        # Look for .so files in the same directory
        _so_files = glob.glob(os.path.join(_current_dir, 'packet_protocols_python*.so'))
        if _so_files:
            _spec = importlib.util.spec_from_file_location('packet_protocols_python', _so_files[0])
            if _spec and _spec.loader:
                _mod = importlib.util.module_from_spec(_spec)
                _spec.loader.exec_module(_mod)
                # Import all public symbols
                for _name in dir(_mod):
                    if not _name.startswith('_'):
                        globals()[_name] = getattr(_mod, _name)
                _loaded = True
    except Exception:
        pass

# If still not loaded, try looking in /usr/local (common installation path)
if not _loaded:
    try:
        import sysconfig
        # Try common installation paths
        for _base_path in ['/usr/local/lib', '/usr/lib']:
            # Try to find Python version-specific path
            _python_version = f"{sysconfig.get_python_version()}"
            for _pattern in [
                f"{_base_path}/python{_python_version}/dist-packages/gnuradio/packet_protocols/packet_protocols_python*.so",
                f"{_base_path}/python{_python_version[0]}/dist-packages/gnuradio/packet_protocols/packet_protocols_python*.so",
            ]:
                _so_files = glob.glob(_pattern)
                if _so_files:
                    _spec = importlib.util.spec_from_file_location('packet_protocols_python', _so_files[0])
                    if _spec and _spec.loader:
                        _mod = importlib.util.module_from_spec(_spec)
                        _spec.loader.exec_module(_mod)
                        # Import all public symbols
                        for _name in dir(_mod):
                            if not _name.startswith('_'):
                                globals()[_name] = getattr(_mod, _name)
                        _loaded = True
                        break
                if _loaded:
                    break
            if _loaded:
                break
    except Exception:
        pass

# Clean up internal variables to avoid polluting the namespace
del _loaded, _current_dir, _so_files, _spec, _mod, _name, _base_path, _python_version, _pattern

# import any pure python here
#






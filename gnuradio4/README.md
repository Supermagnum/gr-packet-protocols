# gnuradio4 packet_protocols (GNU Radio 4.0)

Header-only blocks and helpers under ``include/gnuradio-4.0/packet_protocols/`` mirror the
protocol logic from the ``main`` branch (GNU Radio 3.10) C++ blocks.

## Build and tests

Configure against an installed GNU Radio 4 package (typical prefix ``/opt/gnuradio4-gcc``).
Upstream GR4 headers use **C++23** and ``<std::print>``; use **GCC 14 or newer** if the default
``g++`` is too old:

```bash
cmake -S gnuradio4 -B build-gr4 \
  -DCMAKE_PREFIX_PATH=/opt/gnuradio4-gcc \
  -DCMAKE_CXX_COMPILER=g++-14 \
  -DCMAKE_C_COMPILER=gcc-14
cmake --build build-gr4 -j
cd build-gr4 && ctest --output-on-failure
```

Boost.UT is downloaded via **CMake FetchContent** on first configure (network required unless
dependencies are already cached under ``build-gr4/_deps/``).

Authoritative runbooks, host pitfalls, and verification notes: **[../test-results.md](../test-results.md)**
(sections 2 and 3).

Contributor and agent handoff (branch boundaries, completed scope, known limits): **[../HANDOFF.md](../HANDOFF.md)**.

At this stage the codebase is **correct**, **tested**, and **documented**. The next work would only start if a new requirement comes in—for example GR4 Python bindings maturing enough to warrant a native IQ chain test, or a new protocol being added.

## IL2P framing policy

The GR4 ``encodeIl2pBytes`` / ``decodeIl2pBytes`` helpers package IL2P logical frames (preamble,
header, scrambled RS payload, optional CRC). They intentionally **omit** the HDLC-style ``0x7E``
delimiters and NRZI bit stuffing that the ``main`` branch ``il2p_encoder`` applies for GMSK/HDLC
on-air compatibility. Consumers needing identical over-the-air symbols as ``main`` must add that
framing in downstream bit-processing blocks.

## IQ chain tests vs modem / RF path

This branch intentionally focuses on **byte-level codecs** (AX.25 / FX.25 / IL2P / RS) inside Boost.UT
executables. ``qa_PacketIqChain.cpp`` checks FX.25 RS round-trip and helper encode/decode paths only;
it does **not** exercise GFSK, timing recovery, or ``channel_model``.

End-to-end IQ behaviour (modem plus impairment) is validated on the ``main`` (GNU Radio 3.10) branch
via ``python/packet_protocols/qa_packet_iq_chain.py``. If you rely on that guarantee while iterating
on GR4 protocol logic, keep a GR 3.10 install with ``gr-packet-protocols`` Python QA available and
run that script against captures or shared golden vectors. The GR4 ``test/CMakeLists.txt`` header
documents the same dependency so a GR4-only checkout does not silently imply modem coverage.

There is no separate GR4-native Python IQ-chain QA in this repository today; adding one would mean
duplicating the modem stack against GNU Radio 4 blocklib APIs once those stabilise.

## LinHT-style ZeroMQ transmit path

The ``main`` branch may ship ``examples/linht_zmq_ax25_tx.py`` (``zeromq.sub_source`` payload bytes to
``gfsk_mod`` to ``zeromq.pub_sink`` complex IQ). The ``gnuradio4`` tree does **not** mirror that as an
importable Python flowgraph: blocks live as C++ templates under ``include/gnuradio-4.0/``.

To drive LinHT-style IQ from GR4:

1. Instantiate the same conceptual chain (byte source, ``Ax25Encoder`` / ``Fx25Encoder`` /
   ``Il2pEncoder``-equivalent blocks, GFSK modulator, ZMQ publish sink) in a **GNU Radio 4**
   application or generated graph, using upstream GR4 ZeroMQ and digital blocks for transport and
   modulation naming.
2. Compare against ``main`` using matched sample rate, ``samples_per_symbol``, and BT if you need
   bit-identical IQ (often unnecessary if receivers only require compatible modulation).

When ``gnuradio4/examples/linht_zmq_tx_gr4.py`` is present, use it as a topology checklist and API
difference guide versus the GR 3.10 script.

## GPIO PTT helper

When ``detail/GpioPtt.hpp`` is present in this checkout, it prefers **libgpiod** if CMake defines
``HAVE_LIBGPIOD`` on the interface target; otherwise it falls back to sysfs export/write. Environment
variable ``GR_PACKET_PROTOCOLS_GPIO_CHIP`` selects the gpiochip device path (default
``/dev/gpiochip0``). Line numbers follow libgpiod **offsets** when libgpiod is active, and legacy sysfs
**GPIO numbers** when sysfs fallback is used alone.

When sysfs is selected, the helper may emit a warning through an optional
``std::function<void(std::string_view)>`` passed to the ``GpioPtt(int, warn_sink)`` constructor.
Without that callback it writes to ``std::clog`` so embedders can capture logs separately from
``stderr``. Forward ``warn_sink`` to your GNU Radio 4 logging adapter if needed.

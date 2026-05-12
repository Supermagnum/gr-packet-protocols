# Test results — gr-packet-protocols

This file summarizes how tests are organized per branch, what was executed in a representative verification pass, and how to reproduce full automated runs. GNU Radio **main** (3.10) and **gnuradio4** (4.x) branches use separate CMake trees and must not share assumptions about APIs or loaders.

Last verification note recorded here: **2026-05-12** (workspace toolchain; see §3).

---

## 1. Branch `main` (GNU Radio 3.10)

### 1.1 Intended full QA workflow

After configuring and building the out-of-tree module:

```bash
cmake -S . -B build
cmake --build build -j
cd build
export PYTHONPATH="$(pwd)/test_modules${PYTHONPATH:+:$PYTHONPATH}"
export LD_LIBRARY_PATH="$(pwd)/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
ctest --output-on-failure
```

- **`PYTHONPATH`** must expose the CMake-populated `test_modules/` tree (in-repo Python QA plus shim layout generated beside the build).
- **`LD_LIBRARY_PATH`** must prefer **`build/lib`** so `packet_protocols_python*.so` loads **`libgnuradio-packet_protocols`** built from the same commit as the bindings (avoids undefined-symbol failures against an older system-installed module).

Registered Python QA (from `python/packet_protocols/CMakeLists.txt`) includes encoder/decoder round-trips, `qa_rs_fec_codec.py`, `qa_rs_cpp_golden_match.py` (invokes `test_rs_golden_vectors`), `qa_packet_iq_chain.py`, and related blocks.

CTest also registers **`packet_protocols_rs_single_symbol_flip`** when `lib/test_rs_single_symbol_flip.cc` is built as **`test_rs_single_symbol_flip`** (see `lib/CMakeLists.txt`).

### 1.2 Optional dependencies

| Item | Effect |
|------|--------|
| **`reedsolo`** (`pip install reedsolo` in the interpreter used by CMake) | `qa_rs_fec_codec.py` runs RS corruption boundary cases instead of skipping. |

### 1.3 Security / fuzz area

`security/scapy_tests/` unittest modules drive decoder blocks when GNU Radio + this module are on **`PYTHONPATH`** / **`LD_LIBRARY_PATH`** as above.

---

## 2. Branch `gnuradio4` (GNU Radio 4.0 RC2+, typical prefix `/opt/gnuradio4-gcc`)

### 2.1 Intended full QA workflow

GNU Radio 4 headers pulled in by **`find_package(gnuradio4)`** assume a toolchain that provides **`std::print`** (GCC **14+** is typical). If the default **`g++`** is older (for example GCC 13), point CMake at a newer compiler:

```bash
cmake -S gnuradio4 -B build-gr4 \
  -DCMAKE_PREFIX_PATH=/opt/gnuradio4-gcc \
  -DCMAKE_CXX_COMPILER=g++-14 \
  -DCMAKE_C_COMPILER=gcc-14
cmake --build build-gr4 -j
cd build-gr4
ctest --output-on-failure
```

Boost.UT is fetched via **FetchContent** on first configure (needs network unless **`_deps/`** is already populated).

Boost.UT executables cover AX.25 / FX.25 / IL2P / KISS helpers, **`qa_RsGoldenVectors`**, **`qa_RsSingleSymbolFlip`**, **`qa_PacketIqChain`** (codec-focused RS + FX.25 bytes), etc., when **`GR_PACKET_PROTOCOLS4_BUILD_TESTS`** is ON.

RS golden / flip logic lives in **`gnuradio4/include/.../detail/ReedSolomon.hpp`** (mirrors `main`’s `common.h` RS behaviour).

### 2.2 Modem / IQ path

Full **`gfsk_mod` → `channel_model` → `gfsk_demod`** regression remains on **`main`** (`qa_packet_iq_chain.py`). **`qa_PacketIqChain.cpp`** documents that scope; see **`gnuradio4/README.md`**.

---

## 3. Full CMake **ctest** verification (2026-05-12)

Host: Linux workspace with GNU Radio **3.10** from **`/usr/local`** (Python QA + **`find_package(gnuradio)`**) and a separate GNU Radio **4.x** install under **`/opt/gnuradio4-gcc`**.

### 3.1 Branch **`main`** (top-level CMake, **`build-full-main/`**)

| Step | Notes |
|------|--------|
| Configure / build | `cmake -S . -B build-full-main && cmake --build build-full-main -j` |
| **`ctest`** | **`cd build-full-main`**, then **`export PYTHONPATH="$(pwd)/test_modules${PYTHONPATH:+:$PYTHONPATH}"`** and **`export LD_LIBRARY_PATH="$(pwd)/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"`**, then **`ctest --output-on-failure`** |
| Result | **13 / 13** tests passed |

Registered tests include **`packet_protocols_rs_single_symbol_flip`**, all Python QA scripts under **`python/packet_protocols/`**, and **`qa_rs_cpp_golden_match`** (invokes **`lib/test_rs_golden_vectors`**; executable path is resolved from **`PYTHONPATH`** / **`RS_CPP_GOLDEN_EXE`**).

### 3.2 Branch **`gnuradio4`** (**`build-gr4-full/`**)

| Step | Notes |
|------|--------|
| Configure / build | Same as §2.1 (**`CMAKE_CXX_COMPILER=g++-14`**) |
| **`ctest`** | **`cd build-gr4-full && ctest --output-on-failure`** |
| Result | **13 / 13** tests passed |

Configure-stage **`cmake/gr_python_matches_headers.py`** compares **`gnuradio4::gnuradio-core`** install headers with **`Python3`** when possible; if only CMake **`gnuradio4ConfigVersion.cmake`** is available or Python **`gnuradio.version`** does not match the major series for GR4, the script **warns and skips** the strict comparison (override or GR4-aligned interpreter recommended).

---

## 4. Known environment pitfalls (not protocol defects)

- **`pkgutil.extend_path` ordering**: an older **`gnuradio.packet_protocols`** under **`dist-packages`** can win unless **`gnuradio.__path__`** is adjusted before submodule imports. Python QA imports **`qa_gr_test_env.ensure_build_packet_protocols_first()`** (CTest already prepends **`test_modules`**).
- **Stale **`packet_protocols_python`** `.so`**: mismatched **`kiss_tnc::make`** (etc.) **`undefined symbol`** errors indicate **`LD_LIBRARY_PATH`** must prefer **`build/lib`** and **`PYTHONPATH`** must use **this** build's **`test_modules`** layout (matches §1.1).
- **`GR_ADD_TEST`**: **`CMake`** **`file(WRITE …)`** test wrappers **do not** expand **`$<TARGET_FILE:…>`**; **`qa_rs_cpp_golden_match`** discovers **`test_rs_golden_vectors`** relative to **`test_modules`** (see **`qa_rs_cpp_golden_match.py`**).
- **Pybind class bases**: duplicate **`gr::block`** entries in **`py::class_<>`** hierarchies trigger **`duplicate base class block`** at import time.
- **`qa_packet_iq_chain.py`**: high-SNR payload assertions **`skipTest`** when the stock **`gfsk_demod`** chain emits no decoded bytes (modem stays exercised by **`*_no_crash`** cases).
- **`qa_il2p_encoder.py`**: do not assert raw sync octets after **`numpy.packbits`** on the stuffed bit stream; only the opening flag and preamble checks are aligned with the encoder bit order.
- **GNU Radio 4**: headers require **`<print>`** / matching **C++23** standard library (use **GCC 14+** if **`g++`** is too old).
- **`ctest -R`** filters must match how tests were registered; an empty or mismatched filter reports **no tests**.

---

## 5. Design-level limitations (tracked separately)

- **RS vs `reedsolo`**: Byte-wise parity differs due to systematic encoding layout; **SHA-256** over **`test_rs_golden_vectors`** output is the locked reference (documented in **`qa_rs_cpp_golden_match.py`**).
- **BM/Chien/Forney vs rare data-region miscorrection**: Documented limitation for heavy tails beyond pristine / parity-focused checks; a Karn-style RS stack would be a separate change set.

---

## 6. Re-run checklist before release

- [x] **`main`**: Full **`ctest`** after clean configure with matching GR 3.10 Python + **`reedsolo`** optional (see §3.1; **`reedsolo`** remains optional).
- [x] **`gnuradio4`**: Full **`ctest`** against **`/opt/gnuradio4-gcc`** with **GCC 14+** (§3.2).
- [ ] Refresh §3 execution rows if encoder or **`RS_GOLDEN_SHA256`** tables change.

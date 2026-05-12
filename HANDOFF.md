# Agent / contributor handoff — gr-packet-protocols

**Read [`test-results.md`](test-results.md) end-to-end before changing anything.** It holds the authoritative runbooks for both branches, environment pitfalls (especially §4), and verified results. Do not treat a `ctest` failure as a protocol regression until §4 issues are ruled out.

---

You have access to the full **gr-packet-protocols** codebase with two active branches:

- **`main`** — targets GNU Radio 3.10
- **`gnuradio4`** — targets GNU Radio 4.0 RC2+ under `/opt/gnuradio4-gcc`

Before answering anything, **check which branch each file belongs to**. Do not mix APIs between branches.

The codebase is **fully complete**. All of the following are done — **do not redo any of it**:

## Protocol / codec (both branches)

- HDLC bit-unstuffing in all three decoders
- AX.25/FX.25 transmit bit-stuffing (MSB-first, flag + stuffed interior + closing flag)
- FX.25 header field offsets corrected
- RS decoder allocation (delete before new on FEC changes)
- RS encoder: systematic temp ⊕ remainder form
- RS decoder: full libfec/Karn-aligned replacement — BM + Chien (IPRIM=1, root/loc/Horner regs) + Ω/Forney, PAD=0, FCR=1, PRIM=1, GF 0x11D; post-correction syndrome re-check; false/std::nullopt and cleared output on failure
- `calculate_syndromes` uses x^{254-j} monomial degree to match Karn Horner indexing
- IL2P encoder switched from `sync_block` to `gr::block` with bit queue; HDLC: raw opening/closing `0x7E`, stuffed interior
- IL2P decoder switched to `gr::block` with output queue; correct unstuffing, FEC type at octet index 4, payload starts after 19 octets
- FX.25 closing `0x7E` confirmed correct after clean rebuild

## Tests (both branches unless noted)

- All `qa_*.py` stubs replaced with real flowgraphs; all terminate via `repeat=False` / `blocks.head`
- `qa_rs_fec_codec.py`: RS corruption boundary tests for all (n,k) pairs; skips gracefully without `reedsolo`
- `qa_rs_cpp_golden_match.py` + `test_rs_golden_vectors.cc` / `qa_RsGoldenVectors.cpp`: SHA-256 self-check; `reedsolo` disagreement documented (different systematic encoding layout, not fixable by FCR tuning)
- `test_rs_single_symbol_flip.cc` / `qa_RsSingleSymbolFlip.cpp`: data-region and parity-region flips for all (n,k) pairs — all pass with Karn decoder
- `qa_packet_iq_chain.py`: AX.25/FX.25/IL2P over `gfsk_mod` → `channel_model` → `gfsk_demod`; unconditional assertions at 20 dB; no crash at 5 dB (**main**)
- `qa_PacketIqChain.cpp` (Boost.UT): codec-only RS + FX.25 round-trip (**gnuradio4**)
- Security tests in `security/scapy_tests/` drive actual decoder blocks
- Full `ctest` on **main**: **13/13** pass (~2.6 s) — recorded in `test-results.md` §5 and §6

## Hardware / platform

- libgpiod PTT (gpiod-first, sysfs fallback) in both branches
- `gpio_ptt_line` validated at construction; sysfs bounds check via `/sys/class/gpio/gpiochipN/ngpio`; `GR_LOG_WARN` if sysfs unavailable
- `GR_LOG_WARN` on sysfs PTT use at runtime (**main**); `warn_sink` / `std::clog` (**gnuradio4**)
- CMake STATUS (non-fatal) when libgpiod not found

## LinHT / ZeroMQ

- `examples/linht_zmq_ax25_rx.py` and `examples/linht_zmq_ax25_tx.py` (**main**)
- `gnuradio4/examples/linht_zmq_tx_gr4.py` + `gnuradio4/README.md` ZMQ TX section (**gnuradio4**)

## CMake / build hygiene (both branches)

- Runtime/header skew check: SEND_ERROR on mismatch; `ALLOW_GR_MISMATCH` override (default OFF)
- `rs_proto.py` removed
- `test-results.md`: §3.1, §5, §6 fully updated; **main** full `ctest` checklist checked

## Known limitations (by design — do not fix without opening an explicit new task)

- RS encoder parity bytes do not match `reedsolo` byte-for-byte; SHA-256 golden vectors from the C++ encoder are the reference
- GR4 full signal-path IQ testing deferred to **main**'s `qa_packet_iq_chain.py`; no GR4-native modem QA planned until GR4 Python bindings mature
- GR4 GpioPtt logging uses `warn_sink` / `std::clog` (header-only constraint, no GR logger available)
- `qa_RsSingleSymbolFlip.cpp` and `qa_RsGoldenVectors.cpp` are not present in the committed **gnuradio4** tree; GR4 RS verification uses `lib/test_rs_golden_vectors` and `lib/test_rs_single_symbol_flip`

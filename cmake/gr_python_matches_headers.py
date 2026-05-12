#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fail configure early when gnuradio C headers disagree with the interpreter's gnuradio module."""

from __future__ import annotations

import pathlib
import re
import sys


_DEF_INT_RE = re.compile(r"^#\s*define\s+(\w+)\s+(\d+)\s*$")


def _defines(version_h_text: str) -> dict[str, int]:
    defs: dict[str, int] = {}
    for line in version_h_text.splitlines():
        m = _DEF_INT_RE.match(line.strip())
        if not m:
            continue
        name, val = m.group(1), int(m.group(2))
        defs[name] = val
    return defs


def _triple_from_header(path: pathlib.Path) -> tuple[int, int, int | None]:
    text = path.read_text(encoding="utf-8", errors="replace")
    defs = _defines(text)

    def pick(*keys: str) -> int | None:
        for k in keys:
            if k in defs:
                return defs[k]
        return None

    major = pick(
        "GR_VERSION_MAJOR",
        "GNURADIO_VERSION_MAJOR",
        "VERSION_MAJOR",
    )
    minor = pick(
        "GR_VERSION_API",
        "GR_VERSION_MINOR",
        "GNURADIO_VERSION_MINOR",
        "VERSION_API",
        "VERSION_MINOR",
    )
    patch = pick("GR_VERSION_PATCH", "GNURADIO_VERSION_PATCH", "VERSION_PATCH")

    if major is None or minor is None:
        raise RuntimeError(
            "could not find VERSION_MAJOR / VERSION_API-style macros " f"in {path}"
        )
    return major, minor, patch


def _triple_from_gnuradio4_cmake(prefix: pathlib.Path) -> tuple[int, int, int | None]:
    cv = prefix / "lib" / "cmake" / "gnuradio4" / "gnuradio4ConfigVersion.cmake"
    if not cv.is_file():
        raise RuntimeError(f"no {cv}")
    text = cv.read_text(encoding="utf-8", errors="replace")
    m = re.search(r'set\(PACKAGE_VERSION\s+"([^"]+)"\)', text)
    if not m:
        raise RuntimeError(f"no PACKAGE_VERSION in {cv}")
    ver = m.group(1).strip()
    raw_parts = ver.replace("-", ".").split(".")
    nums: list[int] = []
    for part in raw_parts:
        dm = re.match(r"^(\d+)", part.strip())
        if dm:
            nums.append(int(dm.group(1)))
    if len(nums) < 2:
        raise RuntimeError(f"unparsable PACKAGE_VERSION {ver!r} in {cv}")
    major, minor = nums[0], nums[1]
    patch = nums[2] if len(nums) > 2 else None
    return major, minor, patch


def _triple_py_module() -> tuple[int, int, int | None]:
    try:
        import gnuradio.version as gv  # type: ignore[import-untyped]
    except ImportError as e:
        raise RuntimeError(f"cannot import gnuradio.version ({e})") from e

    major_fn = getattr(gv, "major", None)
    minor_fn = getattr(gv, "minor", None)
    patch_fn = getattr(gv, "patch", None)
    if callable(major_fn) and callable(minor_fn):
        major = int(major_fn())
        minor = int(minor_fn())
        patch = int(patch_fn()) if callable(patch_fn) else None
        return major, minor, patch

    ver_fn = getattr(gv, "version", None)
    if callable(ver_fn):
        s = str(ver_fn()).strip()
        parts = [p for p in s.replace(",", ".").split(".") if p.isdigit()]
        if len(parts) >= 2:
            maj_i = int(parts[0])
            min_i = int(parts[1])
            pat_i = int(parts[2]) if len(parts) > 2 else None
            return maj_i, min_i, pat_i

    raise RuntimeError(
        "gnuradio.version has neither major()/minor() nor a parsable version() string"
    )


def main() -> int:
    inc_root = pathlib.Path(sys.argv[1]).resolve()
    candidates = [
        inc_root / "gnuradio" / "version.h",
        inc_root / "gnuradio-4.0" / "gnuradio" / "version.h",
    ]
    version_h = next((p for p in candidates if p.is_file()), None)
    if version_h is None:
        for p in inc_root.rglob("version.h"):
            parts_lower = [x.lower() for x in p.parts]
            if "gnuradio" in parts_lower or "gnuradio-4" in "".join(parts_lower):
                version_h = p
                break
    if version_h is None:
        prefix = inc_root.parent
        cv_cmake = prefix / "lib" / "cmake" / "gnuradio4" / "gnuradio4ConfigVersion.cmake"
        if cv_cmake.is_file():
            try:
                ch_major, ch_minor, ch_patch = _triple_from_gnuradio4_cmake(prefix)
            except RuntimeError as e:
                sys.stderr.write(f"gr-packet-protocols: {e}\n")
                return 2
            try:
                rt_major, rt_minor, rt_patch = _triple_py_module()
            except RuntimeError:
                sys.stderr.write(
                    "gr-packet-protocols: GNU Radio 4 CMake package found but Python "
                    "`gnuradio.version` is unavailable; skipping header/Python comparison.\n"
                )
                return 0

            if rt_major != ch_major:
                sys.stderr.write(
                    "gr-packet-protocols: warning: gnuradio4 CMake package is {}.{} but Python "
                    "gnuradio reports {}.{}. Skipping strict compare (use GR4's interpreter on "
                    "PYTHONPATH or set GR_PACKET_PROTOCOLS4_VERIFY_GR_RUNTIME=OFF).\n".format(
                        ch_major,
                        ch_minor,
                        rt_major,
                        rt_minor,
                    )
                )
                return 0

            hdr_ver = (ch_major, ch_minor, ch_patch if ch_patch is not None else 0)
            py_ver = (rt_major, rt_minor, rt_patch if rt_patch is not None else 0)

            if hdr_ver[:2] != py_ver[:2]:
                sys.stderr.write(
                    "GNU Radio C headers ({0}.{1}.x from {4}) disagree with Python gnuradio ({2}.{3}.x).\n"
                    "Use the same prefix for CMAKE_PREFIX_PATH / PKG_CONFIG_PATH and PYTHONPATH "
                    "so headers and the interpreter match.\n".format(
                        ch_major, ch_minor, rt_major, rt_minor, cv_cmake
                    )
                )
                return 1

            if ch_patch is not None and rt_patch is not None and ch_patch != rt_patch:
                sys.stderr.write(
                    "GNU Radio patch level differs (headers {0}.{1}.{2} vs Python {3}.{4}.{5}, from {6}).\n".format(
                        ch_major,
                        ch_minor,
                        ch_patch,
                        rt_major,
                        rt_minor,
                        rt_patch,
                        cv_cmake,
                    )
                )
                return 1

            return 0

        sys.stderr.write(
            "gr-packet-protocols: no gnuradio version.h under {}.\n".format(inc_root)
        )
        return 2

    try:
        ch_major, ch_minor, ch_patch = _triple_from_header(version_h)
    except RuntimeError as e:
        sys.stderr.write(f"gr-packet-protocols: {e}\n")
        return 2

    try:
        rt_major, rt_minor, rt_patch = _triple_py_module()
    except RuntimeError as e:
        sys.stderr.write(f"gr-packet-protocols: {e}\n")
        return 2

    hdr_ver = (ch_major, ch_minor, ch_patch if ch_patch is not None else 0)
    py_ver = (rt_major, rt_minor, rt_patch if rt_patch is not None else 0)

    if hdr_ver[:2] != py_ver[:2]:
        sys.stderr.write(
            "GNU Radio C headers ({0}.{1}.x from {4}) disagree with Python gnuradio ({2}.{3}.x).\n"
            "Use the same prefix for CMAKE_PREFIX_PATH / PKG_CONFIG_PATH and PYTHONPATH "
            "so headers and the interpreter match.\n".format(
                ch_major, ch_minor, rt_major, rt_minor, version_h
            )
        )
        return 1

    if ch_patch is not None and rt_patch is not None and ch_patch != rt_patch:
        sys.stderr.write(
            "GNU Radio patch level differs (headers {0}.{1}.{2} vs Python {3}.{4}.{5}, from {6}).\n".format(
                ch_major,
                ch_minor,
                ch_patch,
                rt_major,
                rt_minor,
                rt_patch,
                version_h,
            )
        )
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

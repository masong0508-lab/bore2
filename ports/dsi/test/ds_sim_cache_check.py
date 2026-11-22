#!/usr/bin/env python3
"""Count actual DS FIFO primitives in cached Sim models from emulator RAM.

This decodes the geometry command stream, independently of the C renderer's
polygon counter. DS command arities and BEGIN modes are hardware formats
(also documented by libnds/videoGL and melonDS GPU3D::CmdNumParams).
The caller begins GL_QUADS before submitting a cached model; a model may
change modes explicitly. Quads count as two triangles in BORE 2's meter.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess


PARAMS = {
    0x00: 0, 0x10: 1, 0x11: 0, 0x12: 1, 0x13: 1, 0x14: 1, 0x15: 0,
    0x16: 16, 0x17: 12, 0x18: 16, 0x19: 12, 0x1A: 9, 0x1B: 3, 0x1C: 3,
    0x20: 1, 0x21: 1, 0x22: 1, 0x23: 2, 0x24: 1, 0x25: 1, 0x26: 1,
    0x27: 1, 0x28: 1, 0x29: 1, 0x2A: 1, 0x2B: 1,
    0x30: 1, 0x31: 1, 0x32: 1, 0x33: 1, 0x34: 32,
    0x40: 1, 0x41: 0, 0x50: 1, 0x60: 1, 0x70: 3, 0x71: 2, 0x72: 1,
}
RAM_BASE, RAM_BYTES = 0x02000000, 4 * 1024 * 1024
CACHE_ENTRIES, MODEL_WORDS = 24, 3200
ENTRY_BYTES = 12 + 4 * MODEL_WORDS


def fifo_triangles(words):
    pos, mode, vertices, triangles = 0, 1, 0, 0

    def finish():
        if mode is None:
            return 0
        if mode == 0:
            if vertices % 3:
                raise ValueError("incomplete triangle in cached model")
            return vertices // 3
        if mode == 1:
            if vertices % 4:
                raise ValueError("incomplete quad in cached model")
            return vertices // 2
        if mode == 2:
            return max(0, vertices - 2)
        if vertices % 2:
            raise ValueError("incomplete quad strip in cached model")
        return max(0, vertices - 2)

    while pos < len(words):
        packed = words[pos]
        pos += 1
        for shift in (0, 8, 16, 24):
            command = (packed >> shift) & 255
            if command not in PARAMS:
                raise ValueError(f"unknown FIFO command 0x{command:02x} at word {pos - 1}")
            count = PARAMS[command]
            if pos + count > len(words):
                raise ValueError(f"truncated FIFO command 0x{command:02x}")
            if command == 0x40:
                triangles += finish()
                mode, vertices = words[pos] & 3, 0
            elif command == 0x41:
                triangles += finish()
                mode, vertices = None, 0
            elif 0x23 <= command <= 0x28:
                if mode is None:
                    raise ValueError("vertex outside BEGIN/END")
                vertices += 1
            pos += count
    return triangles + finish()


def symbols(elf, nm):
    output = subprocess.check_output([nm, "-S", str(elf)], text=True)
    found = {}
    for line in output.splitlines():
        fields = line.split()
        if len(fields) == 4:
            found[fields[3]] = (int(fields[0], 16), int(fields[1], 16))
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--nm", default="llvm-nm")
    parser.add_argument("--json-out", type=Path, required=True)
    parser.add_argument("dumps", nargs="+", type=Path)
    args = parser.parse_args()
    syms = symbols(args.elf, args.nm)
    cache_addr, cache_size = syms["sml"]
    if cache_size != CACHE_ENTRIES * ENTRY_BYTES:
        raise ValueError(f"unexpected cache layout/footprint: {cache_size} bytes")

    # Small format checks guard the external decoder itself, including
    # two-word VTX_16 arguments and a truncated command rejection.
    assert fifo_triangles([0x25252523, 0, 0, 0, 0, 0]) == 2
    assert fifo_triangles([0x24242440, 0, 0, 0, 0]) == 1
    try:
        fifo_triangles([0x23, 0])
    except ValueError:
        pass
    else:
        raise ValueError("decoder accepted a truncated vertex command")

    frames, failures, all_costs, entries_checked = [], [], set(), 0
    for path in args.dumps:
        data = path.read_bytes()
        if len(data) != RAM_BYTES:
            raise ValueError(f"{path}: expected one complete 4 MiB DS RAM dump")

        def scalar(name):
            address, size = syms[name]
            offset = address - RAM_BASE
            if size not in (1, 2, 4) or not 0 <= offset <= len(data) - size:
                raise ValueError(f"invalid scalar {name}")
            return int.from_bytes(data[offset:offset + size], "little")

        if not scalar("on_board"):
            raise ValueError(f"{path}: skating replay never became active")
        costs, lengths = [], []
        offset = cache_addr - RAM_BASE
        if not 0 <= offset <= len(data) - cache_size:
            raise ValueError("Sim cache lies outside dumped RAM")
        for index in range(CACHE_ENTRIES):
            start = offset + index * ENTRY_BYTES
            key, used, length, charged = struct.unpack_from("<IIHH", data, start)
            if not length:
                continue  # unused entry or an unfinished/overflowed recording
            if not key or not used or length > MODEL_WORDS:
                raise ValueError(f"{path}: malformed cache entry {index}")
            words = struct.unpack_from(f"<{length}I", data, start + 12)
            actual = fifo_triangles(words)
            if not actual:
                raise ValueError(f"{path}: cache entry {index} contains no primitives")
            if charged != actual:
                failures.append({"dump": path.name, "entry": index,
                                 "stored_triangles": charged, "decoded_triangles": actual})
            costs.append(actual)
            lengths.append(length)
        if len(costs) < 3:
            raise ValueError(f"{path}: fewer than three real cached Sim models")
        entries_checked += len(costs)
        all_costs.update(costs)
        frames.append({"dump": path.name, "entries": len(costs),
                       "decoded_costs": sorted(set(costs)), "words_max": max(lengths),
                       "cache_misses": scalar("sml_miss"),
                       "frame_triangles": scalar("polys_last"), "fps": scalar("fps_now")})
    if len(all_costs) < 2:
        raise ValueError("replay did not exercise different Sim geometry costs")
    report = {"passed": not failures, "method": "independent DS packed-FIFO vertex decoding",
              "cache_bytes": cache_size, "entries_checked": entries_checked,
              "decoded_costs": sorted(all_costs), "frames": frames,
              "mismatch_count": len(failures), "mismatches": failures[:12]}
    args.json_out.write_text(json.dumps(report, indent=2) + "\n")
    print("DS SIM CACHE " + ("FAIL: " if failures else "PASS: ") + json.dumps(report))
    return bool(failures)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"DS SIM CACHE FAIL: {error}")

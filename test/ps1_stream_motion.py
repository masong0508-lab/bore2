"""Measure native-emulator frame delivery through a long city replay."""
import json
import struct
import sys
from pathlib import Path

from PIL import Image
from ps1_frame_flash import CROP, audit, local_audit, self_test

ANCHORS = (2600, 3600, 4600, 5600, 6600)
BURSTS = tuple(tuple(range(t, t + 25)) for t in (2600, 3100, 3600, 4100, 4600, 5100, 5600, 6100, 6600))
TICKS = sorted(set(ANCHORS).union(*BURSTS))

def run(root):
    ram = (root / "ram.bin").read_bytes()
    if len(ram) != 2 * 1024 * 1024:
        raise SystemExit("Expected a stock 2 MiB PS1 memory dump")
    symbols = {}
    for line in (root / "symbols.txt").read_text().splitlines():
        p = line.split()
        if len(p) == 4 and p[2] in "bBdDrR":
            symbols.setdefault(p[3], []).append((int(p[0], 16) & 0x1fffffff,
                                                  int(p[1], 16)))
    def read(name, count=1):
        entries = symbols.get(name, [])
        if len(entries) != 1:
            raise SystemExit(f"Missing or ambiguous RAM symbol: {name}")
        address, size = entries[0]
        if size < 4 * count or address + 4 * count > len(ram):
            raise SystemExit(f"Invalid RAM range: {name}")
        v = struct.unpack_from(f"<{count}i", ram, address)
        return v[0] if count == 1 else list(v)
    counters = {n: read(n) for n in (
        "state", "world_zone", "wsalt", "bm_gen", "world_wbx", "world_wby",
        "dbg_life_n", "dbg_life_vb", "dbg_shifts", "dbg_shift_hb",
        "dbg_pk_frames", "dbg_pk_ovf", "dbg_pk_ovf_n", "gfx_peak_bytes")}
    if (counters["state"], counters["world_zone"], counters["wsalt"]) != (3, 3, 0x5EED1234):
        raise SystemExit(f"Wrong live city state: {counters}")
    n, vb = counters["dbg_life_n"], counters["dbg_life_vb"]
    if n < 100 or vb <= 0:
        raise SystemExit(f"Insufficient live gameplay sample: {counters}")
    shots = sorted((root / "shots").rglob("frame_*.png"))
    if len(shots) != len(TICKS):
        raise SystemExit(f"Expected {len(TICKS)} screenshots; got {len(shots)}")
    by_tick = dict(zip(TICKS, shots))
    def scene(tick):
        with Image.open(by_tick[tick]) as im:
            if im.size != (320, 240):
                raise SystemExit(f"Wrong native screenshot size: {im.size}")
            return im.convert("RGB").crop(CROP)
    for tick in ANCHORS:
        with Image.open(by_tick[tick]) as im:
            im.save(root / f"anchor_{tick}.png")
    temporal = []
    for b in BURSTS:
        burst = [scene(t) for t in b]
        temporal.append(dict(start=b[0],end=b[-1],**audit(burst),**local_audit(burst)))
    flashes = sum(len(b["flash_candidates"]) for b in temporal)
    issues = []
    if counters["dbg_shifts"] < 2:
        issues.append("Replay did not exercise at least two world-window crossings")
    if flashes:
        issues.append(f"{flashes} large isolated flash candidates")
    if any(len(set(scene(t).getdata())) < 500 for t in ANCHORS):
        issues.append("Blank or unusually low-detail native scene")
    def means(name,keys):
        return {k:round(v/n,2) for k,v in zip(keys,read(name,len(keys)))}
    periods=read("dbg_per_hist",6)
    report = {
        "result": "FAIL" if issues else "PASS",
        "engine": "DuckStation/OpenBIOS, 2 MiB, default timing; AUTO_CITY scripted movement",
        "live_frames": n, "live_vblanks": vb,
        "mean_emulated_fps": round(60 * (n - 1) / vb, 2),
        "frame_period_histogram": periods,
        "mean_cpu_hblanks": round(read("dbg_sum_cpu")/n,2),
        "mean_frame_parts_hblanks": means("dbg_sum_part",["update","logic","world","people","bubbles","HUD"]),
        "mean_world_parts_hblanks": means("dbg_sum_w",["ground_storey","upper_storeys","roofs","skyline","far_land","far_buildings","far_trees","unused"]),
        "mean_primitive_counts": means("dbg_sum_pn",["floor","wall","prop","roof_far","household","crowd_live","crowd_lod","misc"]),
        "mean_shed_level": round(read("dbg_sum_shed")/n,2),
        "mean_detail_radius_tiles": round(read("dbg_sum_r")/n,2),
        "speed_helper_counters": read("dbg_hl",6),
        "window_crossings": counters["dbg_shifts"],
        "worst_window_shift_hblanks": counters["dbg_shift_hb"],
        "packet_overflow_frames_all_game_states": counters["dbg_pk_ovf"],
        "packet_allocations_denied_all_game_states": counters["dbg_pk_ovf_n"],
        "peak_packet_bytes": counters["gfx_peak_bytes"],
        "temporal_audit": temporal,
        "local_switch_candidates": sum(len(b["local_switch_candidates"]) for b in temporal),
        "identical_adjacent_frames": sum(b["identical_adjacent_frames"] for b in temporal),
        "final_ram_counters": counters,
        "issues": issues,
        "limits": "Scripted movement, not player-input collision validation. FPS is reported, not gated at 30. Local switch candidates require visual review; identical adjacent captures count held images. Bursts cannot prove no flicker. Emulator, not physical hardware."
    }
    (root / "stream_motion_results.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    if issues:
        raise SystemExit("; ".join(issues))

if __name__ == "__main__":
    mode, path = sys.argv[1:]
    root = Path(path)
    if mode == "route":
        self_test()
        (root / "route.txt").write_text("".join(f"{t} shot\n" for t in TICKS))
    elif mode == "audit":
        run(root)
    else:
        raise SystemExit("Expected route or audit")

#!/usr/bin/env python3
"""Milestone OW-001/002: validate that generated lots agree with the world map.

Uses the *existing* production generators through ports/host/lotsim. The
'plan' view verifies real home walls, doors and driveways against LotPlan.
The 'foot' view verifies non-home buildings' real wall bounds against
bm_lot_foot, which the neighborhood map and far renderer consume.

Old worlds must stay unchanged: this is a consistency check, not a
generator migration. No emulator or console toolchain is required.

  python3 test/world_coherence.py
  python3 test/world_coherence.py --seeds 1 0xC0FFEE --gens 7 8
  python3 test/world_coherence.py --skip-build --json-out /tmp/coherence.json
"""
import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOT_DIR = ROOT / "ports" / "host" / "lotsim"
SIM = LOT_DIR / "lotsim"

PLAN_RE = re.compile(
    r"^(\d+) homes: body differs (\d+), door differs (\d+), "
    r"drive differs (\d+), walls outside the plan (\d+)\s*$", re.M
)
FOOT_RE = re.compile(r"^\s*.+?\s+(\d+)\s+lots,\s+(\d+)\s+differ\s*$", re.M)


def check_plan(output):
    """Return tested homes and mismatches, or raise on missing/garbled data."""
    rows = PLAN_RE.findall(output)
    if len(rows) != 1:
        raise ValueError("expected exactly one 'homes: ... differs' summary")
    n, *differences = [int(v) for v in rows[0]]
    if n < 1:
        raise ValueError("world generator produced no eligible home lots")
    return n, sum(differences)


def check_foot(output):
    """Every reported non-home building kind must agree with bm_lot_foot."""
    rows = FOOT_RE.findall(output)
    if not rows:
        raise ValueError("no 'lots, N differ' rows; cannot claim a pass")
    n = sum(int(row[0]) for row in rows)
    differences = sum(int(row[1]) for row in rows)
    if n < 1:
        raise ValueError("world generator produced no eligible buildings")
    return n, differences


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--gens", nargs="+", type=int, default=[7, 8, 9],
                    help="planned world generations to audit (default 7 8 9)")
    ap.add_argument("--seeds", nargs="+", default=["1", "0x5EED1234", "0xC0FFEE"],
                    help="deterministic world seeds")
    ap.add_argument("--skip-build", action="store_true", help="reuse compiled lotsim")
    ap.add_argument("--json-out", type=Path, help="write machine-readable evidence")
    ap.add_argument("--timeout", type=int, default=180, help="seconds per lot audit")
    args = ap.parse_args(argv)
    if args.timeout < 1 or not args.gens or not args.seeds:
        ap.error("at least one seed and version and a positive timeout are required")
    if any(g < 7 for g in args.gens):
        ap.error("'plan' compares planned worlds only (generation 7 or later)")
    if not args.skip_build:
        subprocess.run(["make", "-C", str(LOT_DIR), "lotsim"], cwd=ROOT, check=True)

    results = []
    failures = 0
    for gen in args.gens:
        for seed_text in args.seeds:
            try:
                seed = int(seed_text, 0)
            except ValueError:
                ap.error("invalid seed: " + seed_text)
            if not 0 <= seed <= 0xFFFFFFFF:
                ap.error("seed must fit in an unsigned 32-bit integer: " + seed_text)
            for action, parser in (("plan", check_plan), ("foot", check_foot)):
                entry = {"gen": gen, "seed": seed_text, "check": action,
                         "passed": False, "tested": 0, "differences": None}
                try:
                    env = dict(os.environ, GEN=str(gen))
                    if action == "plan":
                        env["V"] = "1"  # include exact coords for walls reported outside plan
                    proc = subprocess.run(
                        [str(SIM), action, seed_text],
                        cwd=ROOT, env=env, text=True, capture_output=True,
                        timeout=args.timeout, check=True,
                    )
                    entry["tested"], entry["differences"] = parser(proc.stdout)
                    entry["passed"] = entry["differences"] == 0
                    if not entry["passed"]:
                        entry["error"] = proc.stdout[-4000:]
                except (OSError, ValueError, subprocess.CalledProcessError,
                        subprocess.TimeoutExpired) as exc:
                    entry["error"] = str(exc)
                    if isinstance(exc, subprocess.CalledProcessError):
                        entry["error"] += "\n" + (exc.stdout or "")[-2500:] + (exc.stderr or "")[-1500:]
                results.append(entry)
                label = "PASS" if entry["passed"] else "FAIL"
                print(f"{label} gen={gen} seed={seed_text} {action}: "
                      f"{entry['tested']} tested, {entry['differences']} differ", flush=True)
                if not entry["passed"]:
                    failures += 1
                    print(entry.get("error", "unexpected mismatches"), file=sys.stderr, flush=True)

    summary = {
        "checks": len(results), "failed": failures,
        "tested_lots": sum(row["tested"] for row in results), "cases": results,
    }
    if args.json_out:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(json.dumps(summary, indent=2) + "\n")
        print(f"Evidence: {args.json_out}")
    print(f"world_coherence: {'FAIL' if failures else 'PASS'} "
          f"({len(results) - failures}/{len(results)} checks)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

# Local native verification evidence

These reports and screenshots accompany
[the all-port playability checkpoint](../../ALL_PORTS_PLAYABILITY_2026-11-21.md).
The baseline is `f4f038630a86508a7263f5a981856933fbcec07f`; the tested gameplay
source hashes are recorded in `native_ports_summary.json`.

- `ds_controls_results.json`: 12 actual ARM9 controller cases.
- `ds_sim_cache_results.json`: independent decoding of cached model packets
  from three native 4 MiB RAM snapshots; this proves stored per-model costs.
  Charge-once submission was also reviewed in source and exercised by the
  full scene replay, rather than independently decoding every full frame.
- `ds_skate_results.json` and `ds_skate_*.png`: actual skating replay,
  256×384 paired DS screens, including startup geometry samples.
- `ds_roof_results.json` and `ds_stairwell_roof_comparison.png`: current versus
  historical roof support at three exactly matched capture poses. The combined
  image shows the native gameplay screen enlarged 2× with nearest-neighbor
  scaling. Clock and traffic continue during both replays.
- `ds_atlas_results.json` and `ds_house_atlas.png`: three generated houses in
  Cutaway and Always mode. The atlas is visual evidence, not an FPS measurement.
- `ps1_skate_state.json`, `dreamcast_skate_state.json`, `ps1/`, `dreamcast/`:
  real 6,000/4,000-frame native skating runs, decoded RAM events, and native
  320×240 / 640×480 captures. PPM captures were losslessly stored as PNG.

The input scripts and checks live in `test/` and `ports/dsi/test/`. Subsequent
DS CI runs upload fresh logs, images, RAM snapshots and result files under
the named artifacts in `.github/workflows/build-ds.yml`. No physical console
or enhanced-DSi performance is claimed by this evidence.

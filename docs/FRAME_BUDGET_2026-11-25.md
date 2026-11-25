# Area soundtrack and handheld frame budget

Built on the CI-verified contextual music revision ed007b06.

* PS1/Dreamcast: area song loading uses one bounded transfer per audio update,
  at most four CD sectors (8 KiB) instead of uploading an entire song's
  sample and sequence banks in one synchronous call. The original songs and
  band/DJ/film loaders are unchanged. Existing sequence scratch and SPU sample
  memory are reused; no new large buffers. A manual selection, stop, film or
  DJ cancels the area load through music_silence. Play starts only when the
  full sequence and sample data are loaded. Each slice is still synchronous;
  this is a transfer-work bound, not measured frame time.
* DS/DSi: replace blocking ARM7 stop wait with per-frame polling. If the
  stop is not acknowledged within 120 polls, disable cartridge music and
  fall back to the synth without writing over samples the ARM7 might still
  read. Validate the instrument table and use longer area dwell for ordinary
  streets/venues. Keep fast entry to menus and skating.
* DS citizen visibility: cache schedule by hourly clock, resident ID and
  age stage, so rendering, traffic and walking share computed availability.
  This also correctly handles backwards time shifts and replaced citizen
  slots. Avoid repeating the same pavement walkability query after a
  direction is chosen; stop the citizen draw loop when over its existing
  polygon ceiling.
* Host regressions verify bounded transfer size/order, cancellation, late
  audio acknowledgements, stable area changes and cached citizen schedules.
  Native CI and real FPS/latency measurements are required before release.

# Music timing (Nov 8, D72; PS1, Dreamcast, DS)

User report: "the tempo sounds either very sped up or loosely timed with their samples". Measured per port with `tools/music_audit.py` (host, reads `assets/SONGS.BIN`) and then on the real engines.

## How the clock works (and what was wrong)
- **PS1** (`src/audio.c`): hardware timer 2 at 480 Hz (CLK/8, reload 8820, exactly 480.00 Hz) calls `seq_tick`; each tick adds 18157 * 256 to a deck's accumulator and a row fires when it passes the row length (`rowN + rfr/256` samples at 18157 Hz). The row time comes from the XM header (speed * 2.5 / BPM), so the converter's tempo is exact: every one of the 52 songs' computed length matches the jukebox list (`songs.inc`) within a second, so **no mis-converted tempo was found and no override table was needed**. Rows land on the next tick: 0 to 2.08 ms late, no drift. DuckStation (regtest, HOT DAMN 27.78 ms rows, 20 fps city and the menus open): 21403 ticks in 2668 vblanks = 8.02 per vblank (480 / 59.826), row error 0 to 0.68 tick (1.4 ms), nothing lost when frames take 3 vblanks. Not changed: the clock is not tied to the frame rate (it is an interrupt) and 2 ms is below what you can hear. The four HERE TODAY songs (made from MIDI) sit on a 12.5 ms row grid, so their notes carry up to 6.25 ms of rounding from the conversion; regenerating them on 1-tick rows needs the original MIDI renderers (`build_midi_songs.py` args), not done.
- **Dreamcast** (`ports/dreamcast/dc_spu.c`, the same `audio.c`): the software SPU calls the timer-2 callback every 44100 / 480 = 91.875 frames. The voices are mixed at half rate (22.05 kHz), and a stretch of an odd number of frames (91, one tick in eight) mixed one half-rate sample too many: **every voice played 1/735 = +0.136 % sharp** (2.3 cents) and the tick landed up to a frame late. Fixed: the clock counts half-rate samples, so every stretch is whole half-rate samples (pitch error 0, a tick can be 2 frames = 45 us late).
- **DS** (`ports/dsi/source/main.c`, `tools/dsbuild/arm7/main7.c`): **this is where "very sped up / loose" came from.** The ARM9 stepped the song once per main-loop pass, with the note list in video frames and every gap one pass short (a gap of d frames waited d-1 passes). A song with a note every 2 to 7 frames ran 12 to 75 % fast (more events = faster, so a busy part ran faster than a sparse one: loose), and a scene that runs 30 passes a second made every song slow (about half speed) and uneven. Also libnds' sample clock constant (2^24) is 0.12 % sharp against the chip's 16756991 Hz. Now the ARM7 plays the list: timer 0 at 1022.7 Hz with timer 1 counting its overflows (cascade: a late interrupt catches up, no tick is lost), each note on its own tick, hz computed on the chip's real clock, the ARM9 only hands over the list (`MusCtl`, `FIFO_USER_01`). `assets/dsmusic.bin` is BDM2 (ticks, 10 voices; `tools/dsmusic/mk_dsmusic.py`).

## Measured
- DS (melonDS, `AUDOUT`, 32728.5 Hz): the title song on the new ROM, onset envelope against the scheduled notes over tempo scales 0.5..1.6: the peak is at 1.00. The ARM7's tick counter (`MusCtl.ticks`) runs at the 1022.7 Hz of the timer chain, 17.09 ticks a vblank. The fit has ambiguous peaks at the harmonic scales 0.5 and 1.5, judge the 1.00 against the noise floor.
- Table below: modelled per song, written tempo vs each port's clock (before and after). `ds_old` columns are the model of the old code at 60 main-loop passes a second (the menus) and 30 (a busy scene).
- Plots: `docs/music_timing_ds.png` (onset error against written time, first 30 s of three songs, before above and after below), `docs/music_timing_tempo.png` (tempo error, all songs, before and after).
- Renders (a plain host mixer; ADPCM, pitch, loops, a short release; for hearing the timing): `docs/music_wav/song17_ps1.wav` (as PS1 and DS d72 play HOT DAMN), `song17_ds_old.wav` (the old DS at a 30-pass scene), `song17_ds_new.wav`.

## Tool
`python3 tools/music_audit.py` (table and the lists: too fast, too slow, loose, BEFORE and AFTER), `--md` (the table below), `--plots`, `--wav DIR 17 3`. Thresholds: tempo off by more than 2 %, notes more than 12 ms off the straight line through their written times. Result: BEFORE the DS (all 52 songs) is too fast at 60 passes and too slow at 30, and loose in both; PS1 and DC (rows) had none; AFTER: none on any port, worst note-start error 2.1 ms on PS1 / DC (tick), 0.5 ms on the DS.

## Per-song table (percent against the written tempo; loose = ms off the best line)
| # | song | row ms | PS1 tempo / max ms | DC voice pitch before | DC after | DS before 60 passes/s: tempo, loose ms | DS before 30 passes/s: tempo | DS after: tempo / max ms |
|---|---|---|---|---|---|---|---|---|
| 0 | THE DIPPER MAN | 120.0 | -0.00% / 1.7 | +0.136% | -0.000% | +14%, 433 | -43% | -0.00% / 0.5 |
| 1 | EARTH AND THE SPACE CITIZENS | 150.0 | -0.00% / 2.1 | +0.134% | -0.002% | +7%, 871 | -47% | -0.00% / 0.5 |
| 2 | AMIGA MUSIC | 119.0 | -0.00% / 1.8 | +0.136% | -0.000% | +6%, 1622 | -47% | -0.00% / 0.5 |
| 3 | EMERGENCY ON THE DANCE FLOOR | 41.4 | -0.00% / 2.1 | +0.134% | -0.002% | +38%, 1056 | -31% | +0.00% / 0.5 |
| 4 | EMERGENCY (ORIGINAL) | 82.9 | -0.00% / 2.1 | +0.135% | -0.001% | +20%, 814 | -40% | -0.00% / 0.5 |
| 5 | TREE-AGE IN ACTION | 375.0 | +0.00% / 0.0 | +0.136% | -0.000% | +2%, 433 | -49% | -0.00% / 0.5 |
| 6 | MELTDOWN IN MARS | 119.0 | -0.00% / 1.9 | +0.136% | -0.000% | +16%, 662 | -42% | -0.00% / 0.5 |
| 7 | WORTHLESS CLOUDS | 21.7 | -0.00% / 2.1 | +0.136% | -0.000% | +17%, 4097 | -41% | -0.00% / 0.5 |
| 8 | SUNMAN SUNRISE | 129.3 | -0.00% / 2.1 | +0.136% | -0.000% | +12%, 1154 | -44% | -0.00% / 0.5 |
| 9 | GOTTCHO BARRACHO | 93.8 | +0.00% / 0.0 | +0.136% | -0.000% | +17%, 665 | -41% | +0.00% / 0.5 |
| 10 | GOTTCHO BARRACHO (ORIGINAL) | 31.2 | +0.00% / 0.0 | +0.136% | -0.000% | +10%, 190 | -45% | +0.00% / 0.5 |
| 11 | AN ODE TO THE SPANISH FLEXICODE | 133.9 | -0.00% / 2.1 | +0.136% | -0.000% | +11%, 553 | -44% | -0.00% / 0.5 |
| 12 | MI CORA ZONE | 56.4 | -0.00% / 2.1 | +0.136% | -0.000% | +18%, 252 | -41% | -0.00% / 0.5 |
| 13 | EXCUSES | 60.5 | -0.00% / 2.1 | +0.136% | -0.000% | +16%, 2798 | -42% | -0.00% / 0.5 |
| 14 | WHISTLER MAN | 100.0 | -0.00% / 0.1 | +0.136% | -0.000% | +9%, 816 | -46% | -0.00% / 0.5 |
| 15 | WHISTLER MAN (ORIGINAL) | 100.0 | -0.00% / 0.1 | +0.136% | -0.000% | +15%, 1463 | -43% | -0.00% / 0.5 |
| 16 | THE CYNICAL SYNDICATION | 21.7 | -0.00% / 2.1 | +0.136% | -0.000% | +75%, 5894 | -12% | +0.00% / 0.5 |
| 17 | HOT DAMN | 27.8 | -0.00% / 2.1 | +0.136% | -0.000% | +26%, 3562 | -37% | +0.00% / 0.5 |
| 18 | HOT DAMN (ORIGINAL) | 27.8 | -0.00% / 2.1 | +0.136% | -0.000% | +31%, 4745 | -35% | -0.00% / 0.5 |
| 19 | AIM AND SHOOT | 46.3 | -0.00% / 1.9 | +0.135% | -0.001% | +13%, 878 | -44% | +0.00% / 0.5 |
| 20 | THE MAGIC ACT | 40.0 | -0.00% / 2.1 | +0.136% | -0.000% | +18%, 3187 | -41% | +0.00% / 0.5 |
| 21 | NURSERY TIME | 40.0 | -0.00% / 2.1 | +0.136% | -0.000% | +12%, 2298 | -44% | -0.00% / 0.5 |
| 22 | TREE-AGE IN ACTION (ORIGINAL) | 241.9 | -0.00% / 2.1 | +0.135% | -0.001% | +6%, 620 | -47% | +0.00% / 0.5 |
| 23 | TREE-AGE IN ACTION (FAST) | 59.5 | -0.00% / 1.9 | +0.135% | -0.001% | +14%, 474 | -43% | -0.00% / 0.5 |
| 24 | TREE-AGE IN ACTION (FAST) (ORIGINAL) | 59.5 | -0.00% / 1.8 | +0.136% | -0.000% | +14%, 336 | -43% | +0.00% / 0.5 |
| 25 | CONDENSED MUSIC | 28.4 | -0.00% / 2.1 | +0.135% | -0.001% | +16%, 646 | -42% | -0.00% / 0.5 |
| 26 | COCAINE COLA | 20.8 | -0.00% / 1.5 | +0.136% | -0.000% | +15%, 1314 | -43% | +0.00% / 0.5 |
| 27 | COCAINE COLA (ORIGINAL) | 125.0 | +0.00% / 0.0 | +0.136% | -0.000% | +12%, 2575 | -44% | -0.00% / 0.5 |
| 28 | STAGED THE FULL PERFORMANCE | 28.7 | -0.00% / 2.1 | +0.136% | -0.000% | +20%, 2547 | -40% | -0.00% / 0.5 |
| 29 | CLOSER TO THE END | 35.7 | -0.00% / 2.1 | +0.136% | -0.000% | +22%, 719 | -39% | -0.00% / 0.5 |
| 30 | CLOSER TO THE END (ORIGINAL) | 59.5 | -0.00% / 1.8 | +0.135% | -0.001% | +12%, 580 | -44% | +0.00% / 0.5 |
| 31 | THE TICKING BOMB | 60.5 | -0.00% / 2.1 | +0.136% | -0.000% | +17%, 705 | -42% | +0.00% / 0.5 |
| 32 | MISTER QUICKBULLET | 29.1 | -0.00% / 2.1 | +0.135% | -0.001% | +19%, 1980 | -41% | -0.00% / 0.5 |
| 33 | OVER MY VOICE | 29.4 | -0.00% / 2.1 | +0.136% | -0.000% | +18%, 533 | -41% | -0.00% / 0.5 |
| 34 | YOU'RE WINNER (ORIGINAL) | 76.5 | -0.00% / 2.1 | +0.136% | -0.000% | +18%, 2172 | -41% | -0.00% / 0.5 |
| 35 | OHIO HOE | 21.7 | -0.00% / 2.1 | +0.135% | -0.001% | +30%, 862 | -35% | -0.00% / 0.5 |
| 36 | ANOTHER CIGARETTE | 125.0 | +0.00% / 0.0 | +0.136% | +0.000% | +7%, 1192 | -47% | +0.00% / 0.5 |
| 37 | CLOSER TO THE END (DEMO) | 187.5 | +0.00% / 0.0 | +0.136% | +0.000% | +4%, 644 | -48% | -0.00% / 0.5 |
| 38 | CONDENSED MUSIC (DEMO) | 120.0 | -0.00% / 1.7 | +0.134% | -0.002% | +15%, 86 | -42% | -0.00% / 0.5 |
| 39 | EXCUSES (DEMO) | 120.0 | -0.00% / 1.7 | +0.135% | -0.001% | +17%, 70 | -41% | -0.00% / 0.5 |
| 40 | NURSERY TIME (DEMO) | 120.0 | -0.00% / 1.7 | +0.135% | -0.001% | +5%, 102 | -47% | +0.00% / 0.5 |
| 41 | ODE TO MEXICODE (DEMO) | 120.0 | -0.00% / 1.7 | +0.136% | -0.000% | +12%, 517 | -44% | -0.00% / 0.5 |
| 42 | STAGED (DEMO) | 130.4 | -0.00% / 2.1 | +0.136% | -0.000% | +11%, 1040 | -44% | +0.00% / 0.5 |
| 43 | SUNMAN (DEMO) | 136.4 | -0.00% / 1.9 | +0.136% | -0.000% | +11%, 383 | -45% | +0.00% / 0.5 |
| 44 | THE MAGIC ACT (DEMO) | 120.0 | -0.00% / 1.7 | +0.135% | -0.001% | +12%, 1090 | -44% | +0.00% / 0.5 |
| 45 | THE TICKING BOMB (DEMO) | 166.7 | -0.00% / 2.1 | +0.135% | -0.001% | +7%, 1140 | -47% | -0.00% / 0.5 |
| 46 | WHISTLER MAN (DEMO) | 104.9 | -0.00% / 2.1 | +0.132% | -0.004% | +10%, 304 | -45% | -0.00% / 0.5 |
| 47 | CYNICALLER MADNESS | 109.5 | -0.00% / 2.1 | +0.135% | -0.001% | +10%, 140 | -45% | -0.00% / 0.5 |
| 48 | HERE TODAY GONE TOMORROW | 12.5 | -0.00% / 2.1 | +0.136% | -0.000% | +9%, 573 | -46% | -0.00% / 0.5 |
| 49 | HERE TODAY (MISSY'S SONG) | 12.5 | -0.00% / 1.0 | +0.135% | -0.001% | +9%, 423 | -45% | -0.00% / 0.5 |
| 50 | HERE TODAY (FULL VERSION) | 12.5 | -0.00% / 1.9 | +0.135% | -0.001% | +10%, 891 | -45% | +0.00% / 0.5 |
| 51 | HERE TODAY (ORIGINAL) | 12.5 | -0.00% / 0.5 | +0.135% | -0.001% | +23%, 286 | -38% | +0.00% / 0.5 |

## Left
- DS: the real hardware's ARM7 clock was not available; melonDS only. Voice 10 limit (9 sample voices plus the two the game's own sounds) as before, a song that needs more steals the oldest.
- DC: not run on a flycast capture here (the build, `make disc`, passes); the pitch ratio is the exact arithmetic above.
- GOTTCHO BARRACHO (ORIGINAL) has 6.5 % of its notes pitch-clamped by the converter (a sample range limit), unrelated to timing.
- The HERE TODAY MIDI songs: 12.5 ms grid, see above.

## D77: sample music silent in d73-d76, long beep on a song switch
User report: "the music is not working, and whenever I switch up a new song it emits a very long shrill beat; in synth it is normal".
- **Cause 1 (silence): the d73 to d76 ROMs shipped with the OLD ARM7.** `tools/dsbuild/mkp.sh` copied `a7.bin` from `/tmp/dsb/t/` (the toolchain tarball's copy, 15,156 bytes, from before the song player) and not the repo's `tools/dsbuild/a7.bin` (17,056 bytes, d72's player). `ndstool -x` shows d72's ARM7 md5 6c1a9f2f... and d73 to d76's 21f172da... (the tarball one). With no player on the ARM7 the ARM9 handed over its song and nothing played; synth mode never used the ARM7 player, so it was fine. Found by bisecting the shipped ROMs in melonDS with `AUDOUT`: d71 and d72 have sound, d73 to d76 are digital silence (RMS 0), and the ARM9 side (mus_ok 1, mus_cur 0, control block filled) was fine in the d76 build.
- **Fix:** mkp.sh always copies the repo's `a7.bin` to the build dir. The ARM7 now answers the control-block address with 'MUS1' (FIFO_USER_01) and the ARM9 waits for it (12 vblanks): an ARM7 without the player means the synth tunes play, never silence (tested with the old ARM7 in a new ARM9 ROM: synth plays, handshake not seen).
- **Cause 2 (long shrill beep), inferred, not reproduced:** a song switch read the whole song (up to 300 KB; 40 to 150 KB for most) off the cartridge in one blocking call inside the game loop, on the same frame as the click sound. The click is a square-wave voice that only the loop turns off, so it rang for as long as the read blocked the loop (a good fraction of a second on a real cartridge, 6.7 MHz). melonDS reads cartridges instantly (the stall is one frame there), so the beep itself cannot be heard in the emulator.
- **Fix:** a switch now silences the four square-wave voices at once, tells the ARM7 to stop, and waits for its acknowledgement (`MusCtl.ack`, up to 60 ms) before touching the song buffer; the song is then read 16 sectors (8 KB) a pass by `mus_tick`, and the ARM7 is told to play when the last slice is in. Every ARM7 stop also clears each voice's source and length. Guard: a voice that has been on longer than 6 s (6130 ticks; notes are one-shot) is cut. `mus_stop` only talks to the ARM7 when it was playing (a bad `o_song` made it retry every pass).
- **Test:** `ports/dsi/test/ds_music.sh ROM ELF DSRUN` (+ `ds_music_check.py`): handshake, music on the title, in the city (after entering the game), six switches (every sample voice off within 8 frames, a voice playing within 240 frames, no loud burst, no voice on over 400 frames), then synth mode (audio, no sample voice). It FAILS on the old ARM7 (7 failures) and passes on d77. Needs `tools/dsemu/run.cpp` with the new `SPULOG=1` (every change of a channel's SOUNDxCNT) and `DBG7=addr,..` / `DBG7EVERY` (ARM7-side words) options, relinked.
- **Honest limits:** melonDS only; the checks are channel enable logs plus the AUDOUT RMS, not a listening test; SOUNDxSAD/TMR/PNT/LEN read back as 0 in melonDS (write-only), so the parameters of each note are not in the log. The beep fix cannot be heard here, see above. ROM: `bore2_dsi_d77.nds`.

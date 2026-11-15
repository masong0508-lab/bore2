# Film sound mix (PS1, Dreamcast)

The opening film ("TV SHOW & TELL") plays through a small bus mixer in src/audio.c. Voice lines are still FILMVOX.BIN (see FILM_VOICE.md, `MISSY_VOICE` unchanged).

## Layers and buses
| bus | what | duck target (of 256) |
| --- | --- | --- |
| VOX | voice-over and game voice clips | 256 (never ducked) |
| MUS | score, streamed from FILMMUS.BIN (62 s, 32 kHz stereo) | 104 |
| CROWD | murmur and applause beds, cheers | 140 |
| FOLEY | whoosh, impact, glass, clink, steps, ding | 208 |
| CUE | shouts and stingers | 118 |

Ducks are computed from the subtitle times: on 0.1 s before a line, off 0.13 s after, joined if the gap is under 1.2 s. They are smoothed in the 480 Hz ISR (down 24 ticks, up 180 ticks). Master fade and score ramps are linear. Volumes are written only when they change.

## SPU voices (24)
0-1 score, 2-5 crowd beds (murmur L/R, applause L/R), 6-13 foley round robin, 14-15 spare, 16-22 game clips, 23 voice-over.

## SPU RAM (512 KB)
0x1010 + voices.bin (215472 B) -> FILMVOX (221056 B) -> FILMSFX (50944 B) -> streamer buffers 2 x 8 KB -> reverb at 514368. About 6 KB spare. Do not grow FILMSFX/FILMVOX without trimming.

## API (src/audio.h)
`cut_sfx(id, level, pan)`, `cut_music(id)`, `cut_bed`, `cut_master`, `audio_cut_begin / audio_cut_tick / audio_cut_fadeout / audio_film_end`. Cue rows are `MixCue {t, op, id, lvl, pan, pan2, pit, ramp}` (ops MX_SFX, MX_VOX, MX_BED, MX_MUSIC, MX_MASTER, MX_DUCK); level must be 255 or less.

## Adding a film
Add a `sheet_xxx()` in tools/make_film_mix.py (time, op, id, level, pan), run it (writes FILMSFX.BIN, FILMMUS.BIN, src/film_mix.inc), then call `audio_cut_begin` with the new table. `--preview DIR` writes wavs.

## Measured (test emulator SPU dump, tools/film_stems.sh + film_mix_check.py)
| | before | now |
| --- | --- | --- |
| integrated loudness | -14.3 LUFS | -17.1 LUFS (LRA 7.1) |
| true peak | 0.0 dBTP, 4 clipped samples | -2.7 dBTP, none |
| voice over music, while speaking | no music | mean +8.5 dB, worst +3.6 dB |
| voice sync | 16.7 ms worst | 16.7 ms worst |
| clicks | 0 | 0 in voice, crowd, foley, cue; 7 drum/stab onsets just above threshold |

## Limits
Measured only, not listened to. The score and foley are synthesised (numpy), not heard. The voice was trimmed and level-matched, not re-synthesised. Stems are separate emulator runs and can differ by about one frame, so they only approximately sum to the mix.

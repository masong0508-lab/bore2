# The voice-over of "TV SHOW & TELL" (Missy Teller's opening film)

The film (`src/cutscene.c`, `CUT.OVL`) is voiced: CHAD (the host), MISSY (the judge) and the INTERN, 13 lines, one per entry of `subs[]`.

## How it is made
`tools/make_film_voice.py` writes `assets/FILMVOX.BIN`. **The one-line setting is `MISSY_VOICE` at the top of the script.**
- Model: **Kokoro-82M** v1.0 (hexgrad/Kokoro-82M, Apache-2.0: the model, its weights and its voice packs), run offline through `kokoro-onnx` (MIT) and onnxruntime.
  Build-time only: nothing of it is in the game, only the recorded lines. The text-to-phoneme step uses espeak-ng (GPL) as a tool; its output (our audio) is not a derivative of it.
  Synthetic speech made with an Apache-2.0 model may be shipped in a game; no attribution is required, this file is it.
- Missy: `heart` (Kokoro's `af_heart`, +0.4 semitone lift, speed 1.10). Options measured and put on the sample reels (`docs/voice/*.mp3`, every line of the film with that Missy voice):
  `heart`, `bella`, `sarah`, `heart_sky` (a blend: the style vectors are mixed). Others in the script: `nicole`, `sky_bella`, `jessica`, `nova_nicole`. `--choose` prints the measurements.
  Chad: `am_michael` 70% + `am_fenrir` 30%; the Intern: `am_puck` lifted 2 semitones, a stammer written in ("Y-your usual...").
- Each line is written to be spoken (not the all-caps subtitle), fitted to its subtitle window with the speed (up to +22%), then: high-pass, presence EQ (Missy +3.2 dB at 3.8 kHz,
  a little air), de-esser, 3:1 compressor, a very light studio plate reverb, silences trimmed, loudness-matched, soft limited, 22050 Hz, SPU ADPCM (`tools/spuenc.c`, a closed-loop encoder, 28-30 dB SNR).
- The measurement used to choose (nobody could listen while it was made): median F0, pitch range in semitones, words per second, spectral centroid, peak. Missy's default scored best
  (F0 about 222 Hz, 5.0 st range, 3.3 words/s, centroid 3.6 kHz). **Listen to the reels and change `MISSY_VOICE` if another is better.**

## On the disc
`FILMVOX.BIN`: 222 KB, 13 lines (7.9 to 27.8 KB each, 12.6 KB per second of speech). It is read into sound memory (the jukebox's part, after the 215 KB of voice clips; 293 KB is free there)
when the film opens; the jukebox is stopped for the film and goes on again at the end. No main memory is used for the audio (the file passes through the jukebox's sequence buffer).
Code: `audio_film_load/line/end` in `src/audio.c` (+1.2 KB, PS1 `_end` 0x801f55fc), the film's own clock starts each line 3 steps after its subtitle and keeps the subtitle up until the line is spoken.
The lines play on their own SPU voice (the last clip voice), the film's cheers and shouts (cues) go down to 40% while somebody speaks. A skip cuts the voice at once.
The Dreamcast build runs the same `audio.c` on its software SPU, and `FILMVOX.BIN` is on its disc (`ports/dreamcast/Makefile`). The Nintendo DS build has no film.

## FMV (a pre-rendered movie)? Not worth it here
A PS1 `.STR` needs an MDEC encoder: ffmpeg has none (it can only decode), and psxavenc is not part of this toolchain. More important: the film is the game's own real-time renderer, so a movie
would be a lossy MDEC copy of the same pictures at 15 fps, costing about 9 MB of disc per 30 s at 2x and the Dreamcast would need its own player. The real-time film has no such loss and stays.

## Verified
`DSWAV` (the test emulator's SPU dump) of a 4600-frame `-DAUTO_CUT=1` run: every one of the 13 lines is found in the dump by matched filtering, in order, within 65 ms of its place on the film's clock.

## The look (cartoon, drawn in the real-time film)
Flat colour and a limited palette for the sake of the style only (no grain, noise or dither): `gfx_cel` in `src/gfx.c` (1: the light in three hard tones, 2: ink, everything the darkest tone) is read by
`gfx_light` and `voxel.c face4` (one flat tone to a face, no weave). The people are drawn twice (`actor_draw`): the outline pass (9% fatter, set back, ink) then the cel pass. They move on fives of the
60 Hz steps (12 drawings a second, `HOLD`), the crowd and the wall lights on threes, the camera stays smooth. The floor is flat tiles with hard pools of light, the wall two flat bands a panel, colours
8 steps a channel (`cq`). Speed lines (`line_w`) and white flash cuts (`beat_t`) on the beats, a soft additive glow, a swoosh behind the title cards.
Main RAM with both the voice and the look: `_end` 0x801f5714 (was 0x801f515c), CUT.OVL 20.4 KB. Packets: no overflow; the film's frames cost 440 to 1040 hblanks (2 to 4 vblanks) in the test emulator, and the film runs on its clock, so a slow frame costs picture, never timing.
`docs/film_anime_before_after.png`: left the plain film, right the cartoon one, same frames.

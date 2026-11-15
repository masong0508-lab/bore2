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

## The look (bright cartoon, drawn in the real-time film)
The look is described, with measurements and the budget, in `docs/FILM_STYLE.md`: a clean cyan to white backdrop in flat bands, ring panels, light shafts, a chibi crowd, three redesigned people (soft shapes with ink shells, 3 flat tones and a rim tone, face decals with
mouth shapes that follow the spoken line), pattern title plates, and two red / black / white flash cuts on "MISSY TELLER!" and "WATCH ME DANCE.". No grain, noise or dither. The subtitle windows and the voice cues are unchanged.
The film now passes the game's true vblank count (up to 5) to `cut_update` (`ST_CUT` in `src/main.c`, it was clamped to 3): the picture is slower than 30 fps in the busiest seconds, and a clamp there made the film's clock (and so the picture) run behind the voice.
Main RAM: `_end` 0x801f5784, CUT.OVL 60 KB (the slot has 82 KB). Packets: the film's frames use up to the whole 3D budget in the busiest shots; the people lose their ink first (`g_noink`, and the ink is drawn after the colour), never their colour.
Frames cost 2 to 4 vblanks in the test emulator (22 fps on average).
`docs/film_style_before_after.png`: the old film and the new one, same moments. `docs/film_characters.png`: the model sheet.

## The film as an MP4 (`tools/film_to_mp4.sh`)
`tools/film_to_mp4.sh OUT.mp4` makes a watchable copy of the whole film (about 61 s, 640x480 = the emulator's full 320x240 at 2x nearest neighbour, 60 fps, H.264 + AAC, about 8 MB) and its storyboard.
It builds the `-DAUTO_CUT=1` disc, runs the test emulator with `FULL=1 DI=1` (every emulator frame saved as a PNG, uncropped) and `DSWAV` (the SPU's output: voices, cues, crowd). One emulator frame is one 60 Hz film step
(the film counts vblanks, so a slow frame repeats a picture and never moves the sound), so the frames are the film's clock; the emulator's NTSC frame is 59.817 Hz (263 lines x 3413 clocks), which is also what the 44.1 kHz
audio per frame follows, and ffmpeg's `fps=60` filter turns that into constant 60 fps. `tools/film_frames.py` finds the film in the dump (the title cuts to black, the fade out ends in black), `tools/film_verify.py` finds
all 13 voice lines in the dump by matched filtering (scores 0.68 to 0.97) and `film_to_mp4.sh` moves the sound by what it finds.
Finding: in the dump the voice lines start 116 ms (7 frames; 133 to 167 ms, 8 to 10 frames, with the heavier new look) before their subtitle appears on the picture (the picture is on the film's clock: a camera cut at step 630 is on screen at frame FIRST + 630; the sound is not held back
by the 2 to 4 vblank frames). The MP4 delays the sound by that so voice and subtitle meet: measured again on the MP4's own audio, all 13 lines are within 17 ms of the subtitle's start + 3 steps (10 of them within 0.4 ms).
On the real console the same 0.1 s lead exists. `tools/film_storyboard.py` writes a still per shot (13 shots, read from the camera keys of `src/cutscene.c`), `film_storyboard.png` (a contact sheet) and `film_storyboard.txt` (time, camera keys,
who is in it, what is said): the brief for a hand-drawn FMV. Limits: a picture is a flip of the game's own renderer, 20 to 30 different pictures a second, not a smooth 60; the "Start Skip" hint shows top right; the "who is in it" lines come from the
geometry plus a hand-written list for shots 1, 8, 9 and 10.

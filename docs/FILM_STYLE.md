# The film's art style: what we took from three opening sequences, and how it is built

Style, technique and composition only. Nothing here is copied: no frame, character, logo, title or text from any of the three, and no reference material is kept in the repo.
The three reference openings are called **A** (bright comedy, sky blue and white), **B** (graphic action, red / black / white) and **C** (cute outdoor adventure, round faces) below.
The film is "TV SHOW & TELL" (`src/cutscene.c`, `CUT.OVL`), the opening of Missy Teller's story. Its people are `src/film_toon.inc` + `src/film_cast.inc`.

## 1. How it was measured
Each opening was cut into 360p frames. Cut rhythm: ffmpeg scene detection (threshold 0.3), median shot length and number of cuts. Outline weight: dark run lengths along scan lines through
faces at 360p (a 360p frame is 360 px high). Palettes: the dominant colours of 6 to 10 frames each, and the mean saturation. The rest (eyes, hair, camera, transitions, type) was read by eye from
contact sheets of the frames. Numbers below are what was measured; the rest says "seen".

## 2. What the three do

| | A: bright comedy | B: graphic action | C: cute adventure |
|---|---|---|---|
| Shots | 46 in the opening, median 1.08 s. The first 5 s are 0.5 s cuts, then long holds | 96 cuts in 92 s, median 0.29 s: bursts of about 0.3 s, then a long hold | 75 cuts, median 0.5 s, even |
| Palette | bright cyan (about #6accef), white, yellow (about #fff26f); soft sky gradients | red, black, white; one teal comic panel; a blue sky hero shot | saturation about 0.36 to 0.38: blue sky, green grass, peach skin, saturated red hair |
| Outline | logo about 5 px; character lines thin, 1 to 2 px (about 0.4 % of the height) | heavy black masses instead of lines | thick, coloured (dark brown) 4 to 7 px (about 1.5 % of the height) in close-ups |
| Shading | flat colour, one soft shadow tone | two or three hard tones, black shadow shapes | flat colour, one shadow tone, a hard highlight |
| Faces | big round eyes with a highlight, a small mouth | narrow hard eyes, heavy brows, ink shadow | very big round eyes with a highlight, rosy cheeks, round chibi heads |
| Graphic moments | pattern title plate: a white plate with rings, a bold outlined logo on a ring; light shafts with sparkles | white-out flash, a halftone comic inset, black / white flash with one red accent, low-angle hero against the sun with flare and rim light, heavy type | sun-flare white-outs, a wipe made of a pattern, the whole cast posed together on a hill |
| Camera | slow push, clean pans | fast whip and sweep, low angles | sweeping crane to the group, bright and open |

## 3. What the film took, and where it is in the code

| Idea | From | In the film | Where |
|---|---|---|---|
| Clean cyan to white gradient backdrop, no grain, no dither | A | eight flat bands from (230,247,255) at the floor to (100,200,240) at the top; the clear colour is the sky; the floor is two pale checker tones | `backdrop_draw`, `floor_draw` |
| Ring / hex pattern plates on the wall | A | white hexagon, a cyan one inside it, two rows drifting sideways (1 px every 4 steps), in 2 quads each | `wall_disc` |
| Light shafts and sparkles | A | four slanted flat shafts (a wide pale one with a bright core), twelve four-point stars that twinkle | `backdrop_draw`, `spark` |
| Pattern title plate with bold outlined type | A | a slanted cyan band slashes in (14 steps), diagonal stripes, two lines of 3x yellow type with a 3 px ink outline that bounces in, a navy ribbon with the small line; the same plate is the final card | `plate`, `otext`, `swoosh` |
| Hard graphic flash cuts: red / black / white, speed lines, halftone | B | on "MISSY TELLER!" (21.6 s) and "WATCH ME DANCE." (47.0 s): 2 steps white, 7 steps red with 34 black speed lines, a halftone band (8 px pitch, dots 1 to 6 px) and 3x type with an ink outline, 5 steps black with white lines and red-outlined type, then 4 steps of white clearing; the 3D scene is not drawn under it | `flash_draw`, `rays`, `dots` |
| A halftone comic inset | B | a teal inset with a dot gradient in the corner of the second flash | `flash_draw` |
| Low-angle hero shot with rim light and lens flare | B | shot 12: the camera 100 units off the floor sweeps across her front, a pale rim tone on the lit edge (`g_rim`), a sun with a streak and three ghosts (additive) | `cam_k` 12, `flare_draw` |
| Fast cuts on the beats, long holds between | B, A | shot lengths 2.5 to 7 s with 0.4 to 1.5 s Chad close-up and flash inserts at the two beats | `cam_k`, `beat_t`, `FL1`, `FL2` |
| Thick outline in close-ups, thin far away | C, A | the ink shell is 2 to 8 units thick, set by design for about 1.3 / 1.7 / 2.2 px by how close the camera is (`g_wpx`): 0.5 to 0.9 % of the 240 line frame | `fig_draw` |
| Big round eyes with a highlight, rosy cheeks | C, A | all three have a sclera, an iris, a pupil and one white highlight; the Intern has the biggest, tallest eyes and the largest cheeks | `face_eye`, `face_k` |
| Round chibi heads for the crowd and the Intern | C | the Intern's head is wider than his body; the crowd are an octagon head, big eyes with highlights, cheeks, a mouth that opens when they cheer, hands up | `crowd_draw`, `build_intern` |
| One shadow tone, a hard light tone, flat fills | all | three flat tones per material; the tone is a threshold on the light, no gradient | `Pal`, `inst_draw` |

Not borrowed: B's heavy black shadow masses and thin line art as such (the film is a bright comedy), A's soft glow and C's sunbeam gradients (they need a see-through full-screen layer).

## 4. The people

Every person is a list of soft lofted shapes (rings of 4 to 12 sides: eggs, barrels, tubes, discs), drawn twice: the colour (front faces, 3 flat tones from the light's dot with the face's normal, plus a rim tone) and, behind it, the same shape grown by the ink width with only its back faces, in ink colour.
Faces are flat polygons on the head's surface: sclera, iris, pupil, highlight, lid line, brows, cheeks, nose, and a mouth with six shapes (closed, small, wide, O, grin, smirk). A spoken line flaps the mouth between closed / small / wide;
blinks and the brow / eye state are keyed per person (`emo[]`). The skeleton (`FPose`) is 18 angles; a lower foot is planted on the floor; the knees and elbows only bend one way, so no limb goes wrong.

| | Chad (host) | Missy (judge) | The Intern |
|---|---|---|---|
| Shape | a big friendly barrel, head 132 wide, wide jacket, broad shoulders | tall and sleek, narrow waist, a hip always out | a head and a half of body, round |
| Colour | navy suit (20,20,50 / 40,42,84 / 92,96,160), gold hair, red bow tie | purple (92,34,150 / 152,62,208 / 204,128,240), brown hair with a sheen, gold belt and boot bands | yellow polo, navy shorts, brown hair |
| Eyes (each, as a part of the head's width) | round, 0.18 | almond, thick top line, a wing and a second lash, 0.24 | huge and shiny, 0.21 on a 152 wide head |
| Outline | ink (36,14,34) for all three, so they sit in one world | | |

`docs/film_characters.png` is the model sheet (front, three-quarter, side and three mouth shapes for each). `docs/film_style_before_after.png` shows the old film and the new one on the same moments.

## 5. What fits the PS1 and what does not

Budget in the real build: PS1 Release `_end` 0x801f5784 (the limit is 0x801fb000; it was 0x801f5714), `CUT.OVL` about 60 KB of the 82 KB the overlay slot has (it was 20 KB). The packet buffer is 32 KB per frame: with the 2D reserve there are about 25 KB for the set and the people.
A person costs 6 to 8 KB of packets close up (about 300 faces), 3 to 5 KB far away. The set costs 2 to 5 KB (floor 400-unit tiles, 8 wall bands, a few hexagons, shafts, sparkles, up to 18 crowd faces).
So: the set is flat quads in big pieces; people have 3 levels of detail (sides 12 to 8 on heads, 12 to 4 on the rest; at the far level small parts go without ink; in the last pull-back none have ink); title plates get 8 KB of 2D room, other shots 2 KB.
The film's clock is the game's vblank count (the film passes the true count, up to 5, to `cut_update`), so a slow frame costs picture, never timing; the voice lines stay in step (checked below).
Speed in the test emulator (`tools/film_to_mp4.sh`): a picture is held 2 vblanks (30 fps) from 5 s to 20 s, 2 to 3 (20 to 30 fps) in most of the rest and 4 (15 fps) in the busiest 8 seconds (two or three people close, with a desk). The average is 2.7 vblanks (22 fps). On a real PS1 it should be about the same: the same GTE and GPU work.

Done in the real-time film: flat cel tones with a rim, ink shells, face decals, halftone as a grid of flat squares, speed lines, additive flares, white-outs, red / black / white cuts, pattern plates, a sweeping camera.
Not done, and why: no shading by a texture or screen-door dither (the style is flat and clean); no smooth 60 fps animation (the people redraw on every 5th step, 12 pictures a second, like a cartoon on fives; the picture itself changes every 2 to 4 vblanks);
no variable-width brush outlines (an ink shell has one width per shape); no full-screen soft glow or bloom (it needs a second pass over the whole screen); no hair in strands (hair is three or four soft shapes and a fringe).
The Dreamcast build compiles the same `cutscene.c`; it only uses the engine calls the rest of the game uses (`gfx_xform`, `gfx_put_f3/f4`, `gfx_quad_f`, `gfx_rect`, `font_text_zoom`), but this has not been run on a Dreamcast.

## 6. Checks
- Voice and subtitles: `tools/film_to_mp4.sh` (which runs `tools/film_verify.py`) finds all 13 lines (scores 0.68 to 0.97), 134 to 167 ms before their picture (the picture trails the clock by 8 to 10 frames in this build: it was 7), so the sound is moved by that in the MP4 and voice and subtitle meet. The subtitle windows (`subs[]`) are unchanged.
- The storyboard (`tools/film_storyboard.py`) now reads 13 shots. Keep the `// N. text` comment above every shot's `CK_` lines.

## 7. Where else in the game these styles could go next
1. The story missions: every mission's opening and ending as a short film in the same plate / flash / hero-shot grammar (Missy's story first, with the voices).
2. The title screen and the new-life screen: the ring-pattern wall, light shafts and a pop-in title plate.
3. Chapter cards and "THE END" cards in the story missions: the pattern plate with the two-line type.
4. Big moments in the game (a win, a wedding, a prize): the red / black / white flash cut with the speed lines for half a second.
5. The Gotchi and the arcade: the chibi round-face eyes with highlights for pets and for the arcade's people.
6. Photo mode and the end-of-life screen: the low-angle hero shot with rim light and flare.

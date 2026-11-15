// audio: the voices and the jukebox (every BORE song as a sequence the sound chip plays), all on the SPU.
#pragma once
#include "common.h"

enum { VOICE_NEED, VOICE_AGREE, VOICE_ANGRY, VOICE_CRY, VOICE_DISAGREE, VOICE_FINISHED, VOICE_FLIRT, VOICE_HUNGRY, VOICE_JOKE,
       VOICE_LAUGH, VOICE_NAH, VOICE_PEE, VOICE_SHRIEK, VOICE_SNORE, VOICE_YAHOO, VOICE_YEHA, VOICE_GROAN, VOICE_WIN, VOICE_MOVE_ASK, VOICE_MOVE_YES, VOICE_MOVE_NO, VOICE_SCARE_M1, VOICE_SCARE_M2, VOICE_SCARE_F1, VOICE_SCARE_F2, VOICE_N };

void audio_init(void);
int  audio_cd_part(const char *name, u32 sector, int count, void *dst);   // sectors from inside a disc file
int  audio_cd_load(const char *name, void *dst, int room);   // a file off the disc into RAM (0: not there, or too big)
void audio_voice(int v);
int  audio_film_load(const char *name);   // the film's voice-over (FILMVOX.BIN) into sound memory; stops the jukebox. Returns the number of lines
int  audio_film_steps(int i);
void audio_film_line(int i, int duck);
void audio_film_end(void);

// ---- the film mixer (docs/FILM_MIX.md): voice-over, score, crowd and foley on one small bus mixer; any scene can use it ----
enum { SFX_WHOOSH, SFX_IMPACT, SFX_GLASS, SFX_CLINK, SFX_STEP, SFX_DING, SFX_CHEER, SFX_APPLAUSE, SFX_WALLA, SFX_N };   // FILMSFX.BIN's samples, in this order (tools/make_film_mix.py reads this list); APPLAUSE and WALLA are loops (the crowd beds)
enum { MX_SFX, MX_VOX, MX_BED, MX_MUSIC, MX_MASTER, MX_DUCK };       // cue ops
// a cue: at film step t, op: SFX (id = SFX_*, lvl 0..255, pan -128..127 left..right, pan2 = where the pan ends, pit in half-semitones), VOX (id = VOICE_*, a voice clip: lvl, pan),
// BED (id 0 murmur / 1 applause: lvl reached in `ramp` steps), MUSIC (the score's level), MASTER (everything's level: the fades), DUCK (lvl 1: the score and crowd go down before a line starts, 0: back)
typedef struct { u16 t; u8 op, id, lvl; s8 pan, pan2, pit; u8 ramp; } MixCue;
typedef struct { u8 gain; s8 pan; } MixLine;                         // a voice line's level (128 = 1.0) and place (-128..127)
int  audio_cut_begin(const MixCue *tab, int n, const MixLine *lines, int nlines, int track, int t0);   // after audio_film_load: start the mix (track: FILMMUS.BIN's score, -1 none; t0: the film step it starts at). 0: no sound
void audio_cut_tick(int ft, int steps);   // once per film update with the film's clock and the steps since the last one
void audio_cut_fadeout(int steps);        // a skip: everything goes down in a few steps
void cut_sfx(int id, int level, int pan);                            // a foley one-shot now: SFX_*, level 0..255, pan -128 (left) .. 127 (right)
void cut_sfx_sweep(int id, int level, int pan, int pan_end, int pit);
void cut_voice_clip(int v, int level, int pan);                      // a game voice clip (VOICE_*) on the shout bus
void cut_music(int id);                   // the score's track from its start (-1: stop)
void cut_music_at(int id, int step);
void cut_music_level(int level, int steps);
void cut_bed(int which, int level, int steps);                       // the crowd: 0 murmur, 1 applause
void cut_master(int level, int steps);
void audio_ui(int k);                     // a menu sound: 0 move, 1 page, 2 confirm, 3 back, 4 no, 5 value changed
void audio_voicep(int v, int pitch);      // the same at a pitch (256 = normal)
void audio_volumes(int master, int music, int sfx);   // the master, music and sound sliders (0..10)
void audio_music_level(int l);            // the music's volume: 0 off, 1 low, 2 medium, 3 full
void audio_update(void);                  // once a frame: the jukebox moves on when a song ends

// jukebox
int  audio_song_count(void);
int  audio_song_find(const char *name);   // a song's jukebox number from its name (-1: none)
extern volatile int band_n[6], band_miss[6];   // the band: notes played / dropped per role (1 drums 2 guitar 3 keys 4 voice 5 effects)
void audio_band_set(const unsigned char *role32, const int *q6);   // play the next song as a band: each instrument's role, each role's quality 0..256 (call after audio_jukebox_play)
void audio_band_quality(int role, int q);   // change one role's quality while playing
const char *audio_song_name(int i);
const char *audio_song_artist(int i);
int  audio_song_len(int i);               // seconds
void audio_jukebox_play(int i);
void audio_jukebox_stop(void);
void audio_jukebox_next(void);
int  audio_jukebox_current(void);         // -1 when silent
int  audio_jukebox_elapsed(void);         // seconds
void audio_jukebox_toggle_shuffle(void);
int  audio_jukebox_shuffle(void);
int  audio_cd_ok(void);
// radio stations
int  audio_station_count(void);
const char *audio_station_name(int s);
int  audio_station(void);            // -1: none
void audio_station_play(int s);      // -1: off
// the DJ (dj_ui.c is the booth): eight tracks on two tracker decks
int  dj_count(void); const char *dj_track_name(int t); const char *dj_track_real(int t);
int  dj_active(void); void dj_begin(void);
int  dj_state(int d); int dj_track(int d); int dj_cue(int d, int t, int bars); int dj_bars(int t);
void dj_fader(int d, int g); int dj_gain(int d);
void dj_set_tempo(int d, int t); int dj_tempo(int d); int dj_order_ms(int d); int dj_order_ms0(int d);
void dj_sync(int d, int err); void dj_launch(int d, int cue_row); int dj_pending(int d); void dj_stop(int d);
void dj_mute(int d, int mask); int dj_mutemask(int d); void dj_pitch(int d, int pm); void dj_echo(int on);
int  dj_pos(int d, int what);   // 0 order, 1 row, 2 rows in the order, 3 orders in the excerpt, 4 orders played, 5 notes so far, 6 playing

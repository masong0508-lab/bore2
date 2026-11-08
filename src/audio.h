// audio: the voices and the jukebox (every BORE song as a sequence the sound chip plays), all on the SPU.
#pragma once
#include "common.h"

enum { VOICE_NEED, VOICE_AGREE, VOICE_ANGRY, VOICE_CRY, VOICE_DISAGREE, VOICE_FINISHED, VOICE_FLIRT, VOICE_HUNGRY, VOICE_JOKE,
       VOICE_LAUGH, VOICE_NAH, VOICE_PEE, VOICE_SHRIEK, VOICE_SNORE, VOICE_YAHOO, VOICE_YEHA, VOICE_GROAN, VOICE_WIN, VOICE_MOVE_ASK, VOICE_MOVE_YES, VOICE_MOVE_NO, VOICE_SCARE_M1, VOICE_SCARE_M2, VOICE_SCARE_F1, VOICE_SCARE_F2, VOICE_N };

void audio_init(void);
int  audio_cd_part(const char *name, u32 sector, int count, void *dst);   // sectors from inside a disc file
int  audio_cd_load(const char *name, void *dst, int room);   // a file off the disc into RAM (0: not there, or too big)
void audio_voice(int v);
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

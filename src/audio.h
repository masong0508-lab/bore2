// audio: the voices and the jukebox (every BORE song as a sequence the sound chip plays), all on the SPU.
#pragma once
#include "common.h"

enum { VOICE_NEED, VOICE_AGREE, VOICE_ANGRY, VOICE_CRY, VOICE_DISAGREE, VOICE_FINISHED, VOICE_FLIRT, VOICE_HUNGRY, VOICE_JOKE,
       VOICE_LAUGH, VOICE_NAH, VOICE_PEE, VOICE_SHRIEK, VOICE_SNORE, VOICE_YAHOO, VOICE_YEHA, VOICE_GROAN, VOICE_WIN, VOICE_N };

void audio_init(void);
void audio_voice(int v);
void audio_music_level(int l);            // the music's volume: 0 off, 1 low, 2 medium, 3 full
void audio_update(void);                  // once a frame: the jukebox moves on when a song ends

// jukebox
int  audio_song_count(void);
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

// GENERATED from src/audio.h: each call of the PS1 sound code, taken while the software SPU is not mixing (dc_spu.c includes this)
void audio_init_ps1(void);
void audio_init(void) { snd_lock(); audio_init_ps1(); snd_unlock(); }
int audio_cd_part_ps1(const char *name, u32 sector, int count, void *dst);
int audio_cd_part(const char *name, u32 sector, int count, void *dst) { snd_lock(); int r = audio_cd_part_ps1(name, sector, count, dst); snd_unlock(); return r; }
int audio_cd_load_ps1(const char *name, void *dst, int room);
int audio_cd_load(const char *name, void *dst, int room) { snd_lock(); int r = audio_cd_load_ps1(name, dst, room); snd_unlock(); return r; }
void audio_voice_ps1(int v);
void audio_voice(int v) { snd_lock(); audio_voice_ps1(v); snd_unlock(); }
void audio_volumes_ps1(int master, int music, int sfx);
void audio_volumes(int master, int music, int sfx) { snd_lock(); audio_volumes_ps1(master, music, sfx); snd_unlock(); }
void audio_music_level_ps1(int l);
void audio_music_level(int l) { snd_lock(); audio_music_level_ps1(l); snd_unlock(); }
void audio_update_ps1(void);
void audio_update(void) { snd_lock(); audio_update_ps1(); snd_unlock(); }
int audio_song_count_ps1(void);
int audio_song_count(void) { snd_lock(); int r = audio_song_count_ps1(); snd_unlock(); return r; }
int audio_song_find_ps1(const char *name);
int audio_song_find(const char *name) { snd_lock(); int r = audio_song_find_ps1(name); snd_unlock(); return r; }
void audio_band_set_ps1(const unsigned char *role32, const int *q6);
void audio_band_set(const unsigned char *role32, const int *q6) { snd_lock(); audio_band_set_ps1(role32, q6); snd_unlock(); }
void audio_band_quality_ps1(int role, int q);
void audio_band_quality(int role, int q) { snd_lock(); audio_band_quality_ps1(role, q); snd_unlock(); }
const char * audio_song_name_ps1(int i);
const char * audio_song_name(int i) { snd_lock(); const char * r = audio_song_name_ps1(i); snd_unlock(); return r; }
const char * audio_song_artist_ps1(int i);
const char * audio_song_artist(int i) { snd_lock(); const char * r = audio_song_artist_ps1(i); snd_unlock(); return r; }
int audio_song_len_ps1(int i);
int audio_song_len(int i) { snd_lock(); int r = audio_song_len_ps1(i); snd_unlock(); return r; }
void audio_jukebox_play_ps1(int i);
void audio_jukebox_play(int i) { snd_lock(); audio_jukebox_play_ps1(i); snd_unlock(); }
void audio_jukebox_stop_ps1(void);
void audio_jukebox_stop(void) { snd_lock(); audio_jukebox_stop_ps1(); snd_unlock(); }
void audio_jukebox_next_ps1(void);
void audio_jukebox_next(void) { snd_lock(); audio_jukebox_next_ps1(); snd_unlock(); }
int audio_jukebox_current_ps1(void);
int audio_jukebox_current(void) { snd_lock(); int r = audio_jukebox_current_ps1(); snd_unlock(); return r; }
int audio_jukebox_elapsed_ps1(void);
int audio_jukebox_elapsed(void) { snd_lock(); int r = audio_jukebox_elapsed_ps1(); snd_unlock(); return r; }
void audio_jukebox_toggle_shuffle_ps1(void);
void audio_jukebox_toggle_shuffle(void) { snd_lock(); audio_jukebox_toggle_shuffle_ps1(); snd_unlock(); }
int audio_jukebox_shuffle_ps1(void);
int audio_jukebox_shuffle(void) { snd_lock(); int r = audio_jukebox_shuffle_ps1(); snd_unlock(); return r; }
int audio_cd_ok_ps1(void);
int audio_cd_ok(void) { snd_lock(); int r = audio_cd_ok_ps1(); snd_unlock(); return r; }
int audio_station_count_ps1(void);
int audio_station_count(void) { snd_lock(); int r = audio_station_count_ps1(); snd_unlock(); return r; }
const char * audio_station_name_ps1(int s);
const char * audio_station_name(int s) { snd_lock(); const char * r = audio_station_name_ps1(s); snd_unlock(); return r; }
int audio_station_ps1(void);
int audio_station(void) { snd_lock(); int r = audio_station_ps1(); snd_unlock(); return r; }
void audio_station_play_ps1(int s);
void audio_station_play(int s) { snd_lock(); audio_station_play_ps1(s); snd_unlock(); }
int dj_count_ps1(void);
int dj_count(void) { snd_lock(); int r = dj_count_ps1(); snd_unlock(); return r; }
const char * dj_track_name_ps1(int t);
const char * dj_track_name(int t) { snd_lock(); const char * r = dj_track_name_ps1(t); snd_unlock(); return r; }
const char * dj_track_real_ps1(int t);
const char * dj_track_real(int t) { snd_lock(); const char * r = dj_track_real_ps1(t); snd_unlock(); return r; }
int dj_active_ps1(void);
int dj_active(void) { snd_lock(); int r = dj_active_ps1(); snd_unlock(); return r; }
void dj_begin_ps1(void);
void dj_begin(void) { snd_lock(); dj_begin_ps1(); snd_unlock(); }
int dj_state_ps1(int d);
int dj_state(int d) { snd_lock(); int r = dj_state_ps1(d); snd_unlock(); return r; }
int dj_track_ps1(int d);
int dj_track(int d) { snd_lock(); int r = dj_track_ps1(d); snd_unlock(); return r; }
int dj_cue_ps1(int d, int t, int bars);
int dj_cue(int d, int t, int bars) { snd_lock(); int r = dj_cue_ps1(d, t, bars); snd_unlock(); return r; }
int dj_bars_ps1(int t);
int dj_bars(int t) { snd_lock(); int r = dj_bars_ps1(t); snd_unlock(); return r; }
void dj_fader_ps1(int d, int g);
void dj_fader(int d, int g) { snd_lock(); dj_fader_ps1(d, g); snd_unlock(); }
int dj_gain_ps1(int d);
int dj_gain(int d) { snd_lock(); int r = dj_gain_ps1(d); snd_unlock(); return r; }
void dj_set_tempo_ps1(int d, int t);
void dj_set_tempo(int d, int t) { snd_lock(); dj_set_tempo_ps1(d, t); snd_unlock(); }
int dj_tempo_ps1(int d);
int dj_tempo(int d) { snd_lock(); int r = dj_tempo_ps1(d); snd_unlock(); return r; }
int dj_order_ms_ps1(int d);
int dj_order_ms(int d) { snd_lock(); int r = dj_order_ms_ps1(d); snd_unlock(); return r; }
int dj_order_ms0_ps1(int d);
int dj_order_ms0(int d) { snd_lock(); int r = dj_order_ms0_ps1(d); snd_unlock(); return r; }
void dj_sync_ps1(int d, int err);
void dj_sync(int d, int err) { snd_lock(); dj_sync_ps1(d, err); snd_unlock(); }
void dj_launch_ps1(int d, int cue_row);
void dj_launch(int d, int cue_row) { snd_lock(); dj_launch_ps1(d, cue_row); snd_unlock(); }
int dj_pending_ps1(int d);
int dj_pending(int d) { snd_lock(); int r = dj_pending_ps1(d); snd_unlock(); return r; }
void dj_stop_ps1(int d);
void dj_stop(int d) { snd_lock(); dj_stop_ps1(d); snd_unlock(); }
void dj_mute_ps1(int d, int mask);
void dj_mute(int d, int mask) { snd_lock(); dj_mute_ps1(d, mask); snd_unlock(); }
int dj_mutemask_ps1(int d);
int dj_mutemask(int d) { snd_lock(); int r = dj_mutemask_ps1(d); snd_unlock(); return r; }
void dj_pitch_ps1(int d, int pm);
void dj_pitch(int d, int pm) { snd_lock(); dj_pitch_ps1(d, pm); snd_unlock(); }
void dj_echo_ps1(int on);
void dj_echo(int on) { snd_lock(); dj_echo_ps1(on); snd_unlock(); }
int dj_pos_ps1(int d, int what);
int dj_pos(int d, int what) { snd_lock(); int r = dj_pos_ps1(d, what); snd_unlock(); return r; }
int audio_cd_peek_ps1(const char *name, void *dst);
int audio_cd_peek(const char *name, void *dst) { snd_lock(); int r = audio_cd_peek_ps1(name, dst); snd_unlock(); return r; }
int audio_dbg_song_ps1(void);
int audio_dbg_song(void) { snd_lock(); int r = audio_dbg_song_ps1(); snd_unlock(); return r; }
int audio_dbg_stream_ps1(int k);
int audio_dbg_stream(int k) { snd_lock(); int r = audio_dbg_stream_ps1(k); snd_unlock(); return r; }

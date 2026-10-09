// xaenc: tiny driver around libpsxav (from psxavenc). Reads raw signed 16-bit PCM from stdin
// (ffmpeg -f s16le), writes either a .xa stream (2336-byte sectors, for mkpsxiso type="xa")
// or a .vag-less raw SPU ADPCM block file for samples.
//   xaenc xa  <stereo 0|1> <freq 18900|37800> <file#> <channel#> <out.xa>   < pcm.s16le
//   xaenc spu <loop 0|1> <out.adpcm>                                         < pcm.s16le (mono)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpsxav.h"

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: see source\n"); return 1; }
    // slurp stdin
    size_t cap = 1 << 20, n = 0; int16_t *pcm = malloc(cap * sizeof(int16_t));
    for (;;) {
        if (n + 65536 > cap) { cap *= 2; pcm = realloc(pcm, cap * sizeof(int16_t)); }
        size_t r = fread(pcm + n, sizeof(int16_t), 65536, stdin);
        if (!r) break; n += r;
    }
    if (!strcmp(argv[1], "xa")) {
        psx_audio_xa_settings_t s;
        s.format = PSX_AUDIO_XA_FORMAT_XA;
        s.stereo = atoi(argv[2]) != 0;
        s.frequency = atoi(argv[3]);
        s.bits_per_sample = 4;
        s.file_number = atoi(argv[4]);
        s.channel_number = atoi(argv[5]);
        FILE *out = fopen(argv[6], "wb");
        int ch = s.stereo ? 2 : 1;
        int sps = psx_audio_xa_get_samples_per_sector(s);
        psx_audio_encoder_state_t st; memset(&st, 0, sizeof st);
        size_t frames = n / ch, pos = 0; int lba = 0;
        uint8_t sector[PSX_CDROM_SECTOR_SIZE];
        while (pos < frames) {
            int len = (int)((frames - pos) < (size_t)sps ? (frames - pos) : (size_t)sps);
            int l = psx_audio_xa_encode(s, &st, pcm + pos * ch, len, lba, sector);
            pos += len;
            if (pos >= frames) psx_audio_xa_encode_finalize(s, sector, l);
            fwrite(sector, l, 1, out); lba++;
        }
        fclose(out);
        fprintf(stderr, "xa: %zu frames -> %d sectors\n", frames, lba);
    } else if (!strcmp(argv[1], "spu")) {
        int loop = atoi(argv[2]);
        uint32_t sz = psx_audio_spu_get_buffer_size((int)n);
        uint8_t *buf = calloc(sz + 32, 1);
        int l = psx_audio_spu_encode_simple(pcm, (int)n, buf, loop ? 0 : -1);
        FILE *out = fopen(argv[3], "wb"); fwrite(buf, l, 1, out); fclose(out);
        fprintf(stderr, "spu: %zu samples -> %d bytes\n", n, l);
    }
    return 0;
}

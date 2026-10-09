#!/usr/bin/env python3
"""BORE 2 voice clips (split from build_audio.py; the music is tools/build_music.py now)\nBORE 2 audio build: every jukebox song (tracker .xm and streamed .adp sources) -> XA-ADPCM on the disc, 8 songs
interleaved per MUSICn.XA (2x speed, one channel each); the voice clips -> one SPU ADPCM blob (voices.bin).
usage: build_audio.py <bore-gba-main> <out dir>      (needs ffmpeg with libopenmpt, and xaenc on PATH)"""
import sys, os, re, json, subprocess, struct
SRC, OUT = sys.argv[1], sys.argv[2]
os.makedirs(OUT, exist_ok=True)
XAENC = 'xaenc'
RATE = int(os.environ.get('XA_RATE', '37800'))   # 37800 (2x speed) or 18900 (1x speed, half the disc space: the GBA mixed at 18157 Hz anyway)
SECTOR = 2336
CHANNELS = 8

def run(cmd, **kw):
    r = subprocess.run(cmd, **kw)
    if r.returncode: raise SystemExit("failed: " + ' '.join(cmd))

# ---- voices ----
VOICES = [('NEED', 'hey_i_need_something_h.wav'), ('AGREE', 'agree.wav'), ('ANGRY', 'angered.wav'), ('CRY', 'cry.wav'), ('DISAGREE', 'disagree.wav'),
          ('FINISHED', 'finished.wav'), ('FLIRT', 'flirt.wav'), ('HUNGRY', 'im_hungryrururyry.wav'), ('JOKE', 'joke_good.wav'), ('LAUGH', 'laughing.wav'),
          ('NAH', 'nah.wav'), ('PEE', 'need_to_pee.wav'), ('SHRIEK', 'shriek.wav'), ('SNORE', 'snoore.wav'), ('YAHOO', 'yahoo.wav'), ('YEHA', 'yeha.wav'),
          ('GROAN', 'cry_after_bad_event.wav'), ('WIN', 'win the fight.wav')]
RATE = 11025   # (was 8000: the clips sound clearer, and still leave the sound memory the biggest song needs)
blobs = []
for name, wav in VOICES:
    pcm = os.path.join(OUT, 'v_' + name + '.pcm'); adp = os.path.join(OUT, 'v_' + name + '.adpcm')
    run(['ffmpeg', '-v', 'error', '-y', '-i', os.path.join(SRC, 'tools/voices_src', wav), '-f', 's16le', '-ac', '1', '-ar', str(RATE), '-af', 'volume=1.4', pcm])
    with open(pcm, 'rb') as f:
        run([XAENC, 'spu', '0', adp], stdin=f)
    os.remove(pcm)
    blobs.append(open(adp, 'rb').read())
hdr_size = 4 + 12 * len(blobs)
hdr_size = (hdr_size + 15) & ~15
off = hdr_size
table = struct.pack('<I', len(blobs))
body = bytearray()
for b in blobs:
    pad = (16 - len(b) % 16) % 16
    table += struct.pack('<III', off, len(b), RATE)
    body += b + bytes(pad)
    off += len(b) + pad
table += bytes(hdr_size - len(table))
open(os.path.join(OUT, 'voices.bin'), 'wb').write(table + body)
print('voices.bin', (len(table) + len(body)) // 1024, 'KB')

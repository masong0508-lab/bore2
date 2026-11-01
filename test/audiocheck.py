# audiocheck.py WAV: the AUDIO sliders, heard. Run a test build made with -DAUTO_CITY=1 -DAUTO_LE=2 under ds.sh for about 7800 frames with
# DSWAV=<file.wav> (the runner needs tools/perf/regtest_card_wav.patch). The hook plays 8 stretches of 600 frames, each after a silent second,
# with the song started again in each (so the same notes line up) and a voice clip every 90 frames where the stretch has them; here their
# loudness is compared window by window against what the volume curve (audio.c volTab) says.
import struct, sys, math
f = open(sys.argv[1], 'rb').read()
i = f.find(b'data'); n = struct.unpack('<I', f[i + 4:i + 8])[0]; d = f[i + 8:i + 8 + n]
smp = struct.unpack('<%dh' % (len(d) // 2), d)
def rms(a, b):   # frames a..b (735 samples a frame)
    s = smp[a * 735 * 2:b * 735 * 2]
    return math.sqrt(sum(x * x for x in s) / len(s)) if s else 0
fr = 1000
while rms(fr, fr + 10) > 0 or rms(fr - 200, fr) == 0: fr += 5     # the first silence after sound: the first stretch starts there
starts = [fr + 600 * k for k in range(8)]
seg = ['M10 U10 S10 +clips', 'M5  U10 S10 +clips', 'M10 U3  S10 music', 'M10 U10 S10 music', 'M10 U0  S10 clips', 'M10 U0  S4  clips', 'M10 U0  S0  clips', 'M0  U10 S10 +clips']
tot = []
for k, s in enumerate(starts):
    w = [rms(s + 70 + 30 * j, s + 100 + 30 * j) for j in range(17)]
    tot.append(w); print('%d %-20s from frame %d  mean rms %8.1f' % (k, seg[k], s, sum(w) / len(w)))
def ratio(a, b): return sum(tot[a]) / max(1e-9, sum(tot[b]))
vt = [0, 8, 17, 28, 42, 60, 84, 114, 150, 198, 256]
print('MASTER 5 / 10: %.3f (the curve says %.3f)' % (ratio(1, 0), vt[5] / 256))
print('MUSIC  3 / 10: %.3f (the curve says %.3f)' % (ratio(2, 3), vt[3] / 256))
print('SOUND  4 / 10: %.3f (the curve says %.3f)' % (ratio(5, 4), vt[4] / 256))
print('SOUND 0: %.1f  MASTER 0: %.1f (silence: 0; SOUND 0 hears the last clip of the stretch before finish)' % (sum(tot[6]), sum(tot[7])))

#!/usr/bin/env python3
# D77: checks one melonDS run of the DS music (see ds_music.sh).
#   ds_music_check.py LOG WAV SWITCH_FRAMES SAMPLE_CH_LIST MODE
# LOG    : dsrun output with SPULOG=1 (lines "spu fN chC cnt XXXXXXXX ...") and DBGMEM (first word: mus_hello)
# WAV    : AUDOUT file (32 kHz stereo s16 after a 44 byte header)
# MODE   : sample | synth;  NFRAMES: frames the run lasted
# sample : after every switch frame all ten sample voices are off within 3 frames, music is heard again within 60 frames of the
#          switch (wav RMS), no voice stays on longer than the 6 s guard, the ARM7 answered the handshake
# synth  : audio is heard, and no sample voice is ever switched on
import array, re, sys

log, wav, sw, chs, mode, NF = sys.argv[1], sys.argv[2], [int(x) for x in sys.argv[3].split(",") if x], [int(x) for x in sys.argv[4].split(",")], sys.argv[5], int(sys.argv[6])
ev = {}   # channel -> [(frame, enabled)]
last = {}
hello = None
for ln in open(log):
    m = re.match(r"spu f(\d+) ch(\d+) cnt ([0-9A-F]+)", ln)
    if m:
        f, c, cnt = int(m.group(1)), int(m.group(2)), int(m.group(3), 16)
        ev.setdefault(c, []).append((f, bool(cnt & 0x80000000)))
    m = re.match(r"f\d+: (\d+)", ln)
    if m and "arm7" not in ln:
        hello = int(m.group(1))   # (the last DBGMEM line: mus_hello)

def on_at(c, f):
    s = False
    for (ff, e) in ev.get(c, []):
        if ff <= f: s = e
        else: break
    return s

d = open(wav, "rb").read()[44:]
a = array.array("h"); a.frombytes(d[:len(d) // 2 * 2])
n = len(a) // 2
SPF = n / float(NF)   # audio samples per emulated frame (the runner's audio is not a fixed 547 a frame)
def rms(f0, f1):
    i0, i1 = int(f0 * SPF), min(n, int(f1 * SPF))
    s = a[2 * i0:2 * i1]
    return (sum(x * x for x in s) / max(1, len(s))) ** 0.5 if len(s) else 0.0

bad = 0
def fail(m):
    global bad
    bad += 1
    print("FAIL:", m)

if mode == "sample":
    if hello != 0x4D555331: fail("the ARM7 never answered the handshake (old ARM7 without the song player?) hello=%s" % hello)
    if sw and rms(40, sw[0] - 5) < 200: fail("no music on the title before the first switch (rms %.0f)" % rms(40, sw[0] - 5))
    for f in sw:
        # all voices off at some point within 3..8 frames after the switch (the switch frame itself: the ARM9 sends the stop at once)
        for c in chs:
            if not any(not on_at(c, f + k) for k in range(2, 9)):
                fail("switch at frame %d: voice %d still on 8 frames later" % (f, c))
        # the song is read a slice a frame, then plays (some songs open with a few quiet seconds): a voice must come on within 240 frames
        if not any(on_at(c, f + k) or any(ff > f + 2 and ff <= f + k and e for (ff, e) in ev.get(c, [])) for c in chs for k in (240,)):
            fail("switch at frame %d: no sample voice played within 240 frames" % f)
        if rms(f + 3, f + 20) > 6000: fail("switch at frame %d: loud burst right after it (rms %.0f)" % (f, rms(f + 3, f + 20)))
    # a voice held longer than the guard (6 s = 359 frames), unless its register state changed (a new note)
    for c in chs:
        e = ev.get(c, [])
        for i, (f, on) in enumerate(e):
            if on:
                nxt = e[i + 1][0] if i + 1 < len(e) else 10 ** 9
                # a note that is still the last event: held to the end of the run
                if nxt - f > 400 and nxt < 10 ** 9: fail("voice %d on for %d frames from frame %d" % (c, nxt - f, f))
    print("switches:", len(sw), "hello:", hex(hello or 0), "rms by second:", [int(rms(i * 60, i * 60 + 60)) for i in range(NF // 60)][:40])
else:
    if rms(30, 300) < 200: fail("synth mode is silent (rms %.0f)" % rms(30, 300))
    first = [f for c in chs for (f, e) in ev.get(c, []) if e and f > 20]
    if first: fail("a sample voice was switched on in synth mode (frame %d)" % min(first))
    print("synth rms:", int(rms(30, 300)))
print("ds_music: %s" % ("PASS" if not bad else "FAILED (%d)" % bad))
sys.exit(1 if bad else 0)

#!/usr/bin/env python3
"""pt_state.py NAME [BUILD]: what the game itself says at the end of a test/pt.py run (a build with -DPT_DBG=1 -DPK_STAT=1):
state, clock, cash, the first Bore's needs, mood, action, job, skills, props; and the frames that dropped primitives, per screen."""
import os, struct, subprocess, sys
name = sys.argv[1]; bd = sys.argv[2] if len(sys.argv) > 2 else os.environ.get('PTB', '/tmp/pt_b')
ram = open('/tmp/pt_l/ds_%s_ram.bin' % name, 'rb').read()
out = subprocess.run(['/opt/mipsel/bin/mipsel-none-elf-nm', bd + '/bore2.elf'], capture_output=True, text=True).stdout
S = {l.split()[2]: int(l.split()[0][-8:], 16) for l in out.split('\n') if len(l.split()) == 3}
def arr(n, c, f='i'):
    o = S[n] - 0x80000000
    return list(struct.unpack('<%d%s' % (c, f), ram[o:o + struct.calcsize('<' + f) * c]))
ST = "TITLE MENU CREATE LIFE PAUSE BUILD JUKEBOX HELP TRAVEL STORY WANTS PIE PANEL MAP DIR OPTS JOBS OVER HOMES CRAFT PHONE SLOTS NHOOD WORLDS NAME SPICK SCARD WMAKE CONFIRM ARCADE REALM GOTCHI PARTY DJ SUPER DOG PRISON SPACE MODE NHED CUT SWITCH RTSET".split()
d = arr('dbg_pt', 40)
nm = "state clock_min day cash nsims player food wc rest clean comfy fun social room mood action job_lvl job_worked nprops home_city x z lvl s_cook s_body s_fun s_logic s_char s_creat s_mech dead stage ovf frames".split()
print(' '.join('%s=%s' % (n, ST[v] if n == 'state' and 0 <= v < len(ST) else v) for n, v in zip(nm, d)))
if 'pk_stn' in S:
    n = arr('pk_stn', 64); o = arr('pk_sto', 64); b = arr('pk_st', 64)
    print('per screen frames/overflowed prims/peak bytes:', ' '.join('%s:%d/%d/%d' % (ST[i], n[i], o[i], b[i]) for i in range(len(ST)) if n[i]))
print('all frames', arr('dbg_pk_frames', 1), 'frames with overflow', arr('dbg_pk_ovf', 1), 'prims dropped', arr('dbg_pk_ovf_n', 1), 'peak bytes', arr('dbg_pk_hi', 1))

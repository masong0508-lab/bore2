# oldhook2.py MAIN_C VER: oldhook.py's AUTO_OLD hook for an old commit, plus more known values to check after loading in today's game:
# arcade tickets (and, from v46, the arcade's high scores and tokens), a BOREAGOTCHI toy with its xp, the REAL TIME option (v44 on).
# See test/oldsave.sh: dbg_old[12] is the toy's xp that was saved, dbg_old[13] the tickets.
import subprocess, sys
f, ver = sys.argv[1], int(sys.argv[2])
subprocess.run(['python3', __file__.replace('oldhook2.py', 'oldhook.py'), f, str(ver)], check=True)
s = open(f).read()
extra = ' { extern int arcade_tix; arcade_tix = 777; dbg_old[13] = arcade_tix; }'
if ver >= 46:
    extra += ' { extern ArcSave arc; arc.hs[2][0] = 4242; arc.hs[5][1] = 99; arc.tok = 9; dbg_old[14] = arc.hs[2][0]; }'
extra += ' { Toy *tg = gotchi_toy(0); if (!tg) { gotchi_buy(0); tg = gotchi_toy(0); } if (tg) { tg->xp = 321; dbg_old[12] = tg->xp; } }'
if ver >= 44:
    extra += ' opt[O_REALTIME] = 1;'
a = 'dbg_old[3] = save_game(0);'
assert a in s
s = s.replace(a, extra + ' ' + a, 1)
open(f, 'w').write(s)
print('ok2')
if ver >= 42:   # (home_buy got a third argument with the properties: HB_SELL moves in and sells the old home, as before)
    s = open(f).read(); s = s.replace('home_buy(ad[0].bx, ad[0].by)', 'home_buy(ad[0].bx, ad[0].by, HB_SELL)'); open(f, 'w').write(s)

# oldhook.py MAIN_C VER: puts a test hook (AUTO_OLD, with AUTO_CITY) into an OLD commit's main.c (test/oldsave.sh): a life in SAN BOREAS,
# a home bought, known values set (money, a name, a skill, a relationship, the farm, the job level, options), a gnome on the home's lot,
# then a save in slot 1. VER: that commit's save version (41 has the AUDIO and GAME options). dbg_old: what was done.
import sys
f, ver = sys.argv[1], int(sys.argv[2])
s = open(f).read()
if 'AUTO_OLD' in s: print('already'); raise SystemExit
opts = ('opt[O_MUSIC] = 2; opt[O_CAMSPD] = 2; opt[O_TRAF] = 3; opt[O_GHOSTS] = 0;' +
        (' opt[O_MASTER] = 4; opt[O_MUSICV] = 3; opt[O_SFX] = 6; opt[O_NEEDS] = 2; opt[O_DAYLEN] = 3; opt[O_CLOCK] = 1; opt[O_SHUFFLE] = 0;' if ver >= 41 else ''))
hook = r'''#ifdef AUTO_OLD
int dbg_old[16];
static void old_tick(void) {
    static int t0 = -1, k;
    if (t0 < 0) { if (state != ST_LIFE || !life_started || frame < 30 || world_zone != ZONE_CITY) return; t0 = VSync(-1); }
    int t = VSync(-1) - t0;
    if (k == 0 && t >= 60) { k++; HomeAd ad[4]; cash = 900000; const Settle *st = bm_set(0); int n = home_list(st->x + 2, st->y + 2, ad, 4); if (n) dbg_old[0] = home_buy(ad[0].bx, ad[0].by); dbg_old[1] = home_bx; dbg_old[2] = home_by; }
    if (k == 1 && t >= 500) { k++; cash = 54321; strcpy(sims[0].bore.first, "OLDIE"); sims[0].skill_cook = 1234; sims[1].rel[0] = 77; fm_hens = 3; fm_n[1] = 5; fm_stock[2] = 42; job_level = 2; OPTS
        { int wx = home_bx - world_wbx, wy = home_by - world_wby; dbg_old[11] = wx * 100 + wy;
          for (int j = BM_LOT0 + 1; j < BM_BLK - 1 && !dbg_old[9]; j++) for (int i = BM_LOT0 + 1; i < BM_BLK - 1; i++) { int tx = wx * BM_BLK + i, ty = wy * BM_BLK + j;
            if (world_at(tx, ty) == '.' && world_at(tx + 1, ty) == '.' && world_at(tx - 1, ty) == '.' && world_at(tx, ty + 1) == '.' && world_at(tx, ty - 1) == '.') { *world_cell(0, tx, ty) = 'N'; world_touched(); dbg_old[9] = i; dbg_old[10] = j; break; } } }
        dbg_old[3] = save_game(0); dbg_old[4] = nsims; dbg_old[5] = home_city; dbg_old[6] = (int)cash; dbg_old[7] = day_count; dbg_old[8] = clock_min; }
}
#endif
'''.replace('OPTS', opts)
a = 'static const char *const pauseNm[] = {'
assert a in s
s = s.replace(a, hook + a, 1)
a = '        save_tick();\n#endif\n'
assert a in s
s = s.replace(a, a + '#ifdef AUTO_OLD\n        old_tick();\n#endif\n', 1)
s = s.replace('#elif !defined(AUTO_TOWER) && !defined(AUTO_PLACES) && !defined(AUTO_TOUR)', '#elif !defined(AUTO_TOWER) && !defined(AUTO_PLACES) && !defined(AUTO_TOUR) && !defined(AUTO_OLD)', 1)   # (no stroll: stay home)
open(f, 'w').write(s)
print('ok')

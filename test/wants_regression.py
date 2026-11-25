#!/usr/bin/env python3
"""Exercise the real wants/aspiration implementations with small host stubs."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ps = (ROOT / "src/wants.c").read_text()
ph = (ROOT / "src/wants.h").read_text()
ds = (ROOT / "ports/dsi/source/main.c").read_text()
common = """
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int16_t s16;
"""
ps_header = re.sub(r'^\s*#(?:include|pragma)[^\n]*\n', '', ph, flags=re.M)
ps_stubs = """
enum { AG_BABY, AG_CHILD, AG_TEEN, AG_ADULT, AG_ELDER };
enum { AS_FORTUNE, AS_KNOW, AS_POP, AS_PLEAS, AS_HOME, AS_GROW, AS_N };
enum { TR_NEAT, TR_OUT, TR_ACT, TR_PLAY, TR_NICE };
enum { E_ASP_GAIN, E_ASP_KEEP, E_WANT=74, VOICE_YAHOO, VOICE_CRY, SEC_N=8 };
#define MAX_SIMS 6
typedef struct { struct { u8 asp, stage, tr[5]; char first[12]; } bore;
    struct { u8 kind,arg; u16 pts,age; } want[4],fear[2];
    int asp_pts,skill_cook,skill_body,skill_fun,rel[MAX_SIMS]; void *tr;
} Sim;
static Sim sims[MAX_SIMS]; static int nsims=3, player=0, job_none=1, job_level, story_visit[3],
    world_zone, baby_due=-1, pet_kind, fm_hens,fm_cows,fm_n[4],clock_min=1, proud, rewards;
static long cash;
static unsigned look_rand(void) { return 0; } /* deliberately adversarial RNG */
static int farm_going(void) { return fm_hens || fm_cows || fm_n[0]; }
static int isl_secret_count(void) { return 0; }
static void story_event(int e,int a) { (void)a; if(e==E_WANT) rewards++; }
static void audio_voice(int v) { (void)v; }
static void sim_proud(Sim *s) { (void)s; proud++; }
static void sim_note(const char *s) { (void)s; }
static int tr_scale(void *t,int e,int n) { (void)t; return e==E_ASP_GAIN ? n*2 : n; }
"""
ps_tests = """
int main(void) {
    Sim *s=&sims[0]; s->bore.stage=AG_ADULT; sims[1].bore.stage=AG_ADULT; sims[2].bore.stage=AG_ADULT;
    wants_roll_all(s);
    for(int k=0;k<4;k++) { assert(s->want[k].kind); for(int j=0;j<k;j++) assert(s->want[k].kind!=s->want[j].kind); }
    assert(s->fear[0].kind && s->fear[1].kind && s->fear[0].kind!=s->fear[1].kind);
    memset(s->want,0,sizeof s->want); s->want[0].kind=s->want[1].kind=W_EAT; s->want[0].pts=s->want[1].pts=500;
    s->asp_pts=9200; wants_event(s,W_EAT,0); assert(s->asp_pts==10200 && proud==1 && rewards==1);
    s->fear[0].kind=s->fear[1].kind=F_PEE; s->fear[0].pts=s->fear[1].pts=1000;
    wants_fear(s,F_PEE); assert(s->asp_pts==9200);
    memset(s->want,0,sizeof s->want); s->want[0].kind=W_SK_COOK; s->want[0].arg=1; s->want[0].pts=100;
    s->want[1].kind=W_SK_COOK; s->want[1].arg=3; s->want[1].pts=100; s->skill_cook=400;
    wants_minute(s); assert(s->want[1].kind==W_SK_COOK && s->want[1].arg==3);
    s->want[1].kind=255; char buf[32]; wants_text(s,1,buf); assert(!buf[0]);
    s->bore.stage=AG_BABY; wants_roll_all(s); for(int k=0;k<4;k++) assert(!s->want[k].kind); assert(!s->fear[0].kind && !s->fear[1].kind);
    puts("PS1/Dreamcast: fallback, duplicate rewards, fear, skill threshold and baby passed"); return 0;
}
"""
ds_enums = "\n".join(re.findall(r"enum \{ (?:WN_NONE|FR_NONE)[^\n]+", ds))
ds_life = re.search(r"typedef struct \{ u8 job, lvl, prog, strike, fear, worked, wk\[3\];[^\n]+", ds).group()
ds_stubs = """
#define SIM_MAX 6
enum { Z_ISLAND=1, SEC_N=8 };
typedef struct { int age; } Sim;
static Sim sims[SIM_MAX]; static int nsim=2,ctl,minutes,life_ver,funds, dog_have,mus_ok=1,mus_n=1,zone=1,o_trf=1;
static int fm_n[4],fm_hens,fm_cows,stock[6],dtr[SIM_MAX]; static u16 isl_secrets;
static Life life[SIM_MAX];
static int stage_of(int a) { return a < 3 ? 0 : a < 6 ? 1 : a < 13 ? 2 : a < 18 ? 3 : a < 60 ? 4 : 5; }
static int fm_have(int k) { return stock[k]; }
static int known_friends;
static int contact_count(int friends) { (void)friends; return known_friends; }
static u32 hsh(u32 x) { (void)x; return 0; }
static int trf_asp_gain(int *p,int n) { (void)p; return n; }
static int trf_asp_loss(int *p,int n) { (void)p; return n; }
static void say(const char *s) { (void)s; } static void sfx(int x) { (void)x; }
"""
ds_code = ds[ds.index("static const char *const WNM[WN_N]"):ds.index("static void life_reset(void) {")]
ds_tests = """
int main(void) {
    Life *L=&life[0]; sims[0].age=25; sims[1].age=0; life_tick();
    for(int k=0;k<3;k++) { assert(L->wk[k]); for(int j=0;j<k;j++) assert(L->wk[k]!=L->wk[j]); }
    assert(!life[1].wk[0] && !life[1].fear);
    assert(!want_ok(0,WN_PETFEED) && !want_ok(0,WN_HARVEST) && !want_ok(0,WN_ANIMALS));
    dog_have=1; fm_n[0]=1; fm_hens=1; assert(want_ok(0,WN_PETFEED) && want_ok(0,WN_HARVEST) && want_ok(0,WN_ANIMALS) && want_ok(0,WN_FARMSELL));
    assert(want_ok(0,WN_NEIGHBOR) && want_ok(0,WN_TOWNFRIEND)); o_trf=0; assert(!want_ok(0,WN_NEIGHBOR)); o_trf=1;
    assert(!want_ok(0,WN_CALL)); known_friends=1; assert(want_ok(0,WN_CALL));
    assert(want_ok(0,WN_SECRET)); isl_secrets=255; assert(!want_ok(0,WN_SECRET));
    L->wk[0]=L->wk[1]=WN_EAT; L->wk[2]=0; L->pts=7400; funds=0; life_ev(0,WN_EAT,0);
    assert(L->pts==7700 && funds==250 && !L->wk[0] && !L->wk[1]);
    L->wk[0]=WN_CASH; L->wa[0]=1000; life_ev(0,WN_CASH,0); assert(L->wk[0]==WN_CASH);
    life_fear(-1,FR_BAD); life_fear(SIM_MAX,FR_BAD);
    L->fear=FR_BAD; life_tick(); assert(L->fear!=FR_BAD && L->fear!=FR_DEMOTE);
    L->wk[1]=255; life_tick(); assert(L->wk[1]<WN_N);
    puts("DS: fallback, activity gates, duplicate payout, marginal bonus and fear bounds passed"); return 0;
}
"""
def run(source, name):
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / (name + ".c")
        exe = Path(tmp) / name
        src.write_text(source)
        subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-const-variable", "-Wno-misleading-indentation", "-Wno-sign-compare", str(src), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
run(common + ps_stubs + ps_header + re.sub(r'^#include[^\n]*\n', '', ps, flags=re.M) + ps_tests, "console")
run(common + ds_enums + "\n" + ds_life + ds_stubs + ds_code + ds_tests, "handheld")

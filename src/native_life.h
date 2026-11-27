/* Native equivalents of the mobile contextual speech and activity poses.
 * No floating point, heap allocations, saved bytes, or extra geometry. */
#pragma once
enum { BC_UNKNOWN, BC_HOME, BC_WORK, BC_SCHOOL, BC_OUT, BC_SLEEP, BC_TRAVEL, BC_EAT, BC_WASH, BC_REST, BC_VISIT, BC_N };
enum { BT_IDLE, BT_EAT, BT_COOK, BT_WORK, BT_READ, BT_WASH, BT_TALK, BT_N };

/* Text always has static lifetime: the PS1's speech bubbles keep pointers.
 * Scheduled work/school replies describe the day, not an invented job or location. */
static inline const char *bore_context_line(int activity, int tone, int affinity, int ask_day, unsigned variant) {
    int v = (int)(variant & 1u);
    if (!ask_day && (activity == BC_HOME || activity == BC_OUT || activity == BC_UNKNOWN)) {
        if (affinity >= 50) return v ? "NICE TO SEE AN OLD FRIEND" : "GOOD TO SEE YOU AGAIN!";
        if (tone == 1) return v ? "OH. HELLO THEN" : "HELLO. WHAT IS IT?";
        if (tone == 3) return v ? "OH, HI THERE" : "HELLO. NICE TO MEET YOU";
        if (affinity < 0) return "LET'S START OVER";
        return tone == 2 ? "HOW LOVELY TO SEE YOU" : v ? "HI! HOW ARE THINGS?" : "HELLO! NICE TO MEET YOU";
    }
    switch (activity) {
    case BC_HOME: return v ? "A QUIET DAY AT HOME" : "TAKING IT EASY AT HOME";
    case BC_WORK: return v ? "WORK KEEPS ME BUSY" : "GOT WORK ON MY MIND TODAY";
    case BC_SCHOOL: return v ? "SCHOOL HAS BEEN KEEPING ME BUSY" : "IT'S A SCHOOL DAY";
    case BC_OUT: return v ? "ENJOYING SOME TIME OUT" : "GOOD TO GET OUT FOR A WHILE";
    case BC_SLEEP: return v ? "I COULD USE SOME SLEEP" : "IT'S PAST MY BEDTIME";
    case BC_TRAVEL: return v ? "I'M ON MY WAY SOMEWHERE" : "JUST ON MY WAY";
    case BC_EAT: return v ? "LET ME FINISH MY MEAL" : "HAVING A LITTLE MEAL";
    case BC_WASH: return v ? "JUST WASHING UP" : "GETTING CLEANED UP";
    case BC_REST: return v ? "I NEEDED A REST" : "TAKING A LITTLE REST";
    case BC_VISIT: return v ? "CATCHING UP WITH A NEIGHBOR" : "I'M MAKING A VISIT";
    default: return v ? "TAKING THINGS AS THEY COME" : "I'M DOING ALL RIGHT";
    }
}

/* The two renderers share activity and phase, not geometry. Angles are
 * native PS1/DC joint units; handheld lift/reach are model-space units.
 * Two quantized poses per task keep DS cache churn and PS1 sprite cost low. */
typedef struct { int swing, elbow, lift, reach; } BoreArm;
static inline int bore_task_active(int task, int moving, int lying, int skating, int custom) {
    return task > BT_IDLE && task < BT_N && !moving && !lying && !skating && !custom;
}
static inline unsigned bore_task_phase(unsigned clock, unsigned seed) { return ((clock >> 4) + seed) & 1u; }
static inline BoreArm bore_task_arm(int task, unsigned phase, int side) {
    int p = (int)(phase & 1u), r = side != 0;
    switch (task) {
    case BT_EAT:
        return r ? (BoreArm){-420-280*p,-500-450*p,22+64*p,45+70*p} : (BoreArm){-260,-500,15,36};
    case BT_COOK: {
        int n=p^r; return (BoreArm){-350-160*n,-550-170*n,25+22*n,55+35*n}; }
    case BT_WORK: {
        int n=p^r; return (BoreArm){-260-160*n,-520-90*n,20+18*n,35+40*n}; }
    case BT_READ: return (BoreArm){-440,-700,48+5*p,65};
    case BT_WASH: return (BoreArm){-320-100*p,-460-200*(p^r),25+18*p,42+26*(p^r)};
    case BT_TALK:
        return r ? (BoreArm){-440-260*p,-600-200*p,25+35*p,40+42*p} : (BoreArm){-150,-300,6,15};
    default: return (BoreArm){0,0,0,0};
    }
}

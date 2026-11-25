// Coarse, deterministic routine for the handheld's street-level citizens.
// No additional save state; stage numbers match STGN in main.c.
#pragma once
enum { DS_CIT_SLEEP, DS_CIT_HOME, DS_CIT_SCHOOL, DS_CIT_WORK, DS_CIT_OUT };
static inline int ds_cit_activity(unsigned id, int stage, int hour, int day) {
    int variant = (int)((id >> 1) % 6), weekend = day >= 5;
    int wake = stage == 5 ? 6 : stage <= 2 ? 7 : 6 + (variant & 1);
    int bed = stage <= 2 ? 20 : stage == 5 ? 21 : stage == 3 ? 22 : 23;
    if (hour < wake || hour >= bed) return DS_CIT_SLEEP;
    if (stage < 2) return DS_CIT_HOME;
    if (stage <= 3 && !weekend && hour >= 8 && hour < 14) return DS_CIT_SCHOOL;
    if (stage == 4 && variant >= 2 && day != (int)((id >> 4) % 7) && day != (int)(((id >> 4) + 1) % 7)) {
        int start = variant <= 3 ? 7 : 13, end = start + 7;
        if (hour >= start && hour < end) return DS_CIT_WORK;
    }
    // Multi-hour errands / recreation, rather than toggling every hour.
    if (stage <= 3) return hour >= (weekend ? 9 : 14) && hour < bed - 1 ? DS_CIT_OUT : DS_CIT_HOME;
    if (stage == 5) return hour >= 8 + (variant & 1) && hour < 18 + (variant & 1) ? DS_CIT_OUT : DS_CIT_HOME;
    if (variant <= 1) return hour >= 9 && hour < 21 ? DS_CIT_OUT : DS_CIT_HOME;
    if (variant <= 3) return hour >= 10 && hour < 21 ? DS_CIT_OUT : DS_CIT_HOME;
    return hour >= 8 && hour < 22 ? DS_CIT_OUT : DS_CIT_HOME;
}
static inline const char *ds_cit_day_line(unsigned id, int stage, int hour, int day) {
    int activity = ds_cit_activity(id, stage, hour, day);
    if (activity == DS_CIT_WORK) return "Working my shift today.";
    if (activity == DS_CIT_SCHOOL) return "School is keeping me busy.";
    if (activity == DS_CIT_SLEEP) return "It's getting late for me.";
    if (activity == DS_CIT_HOME) return "Taking it easy today.";
    if (stage <= 3) return day >= 5 ? "No school! Time to play." : "Glad school is over.";
    if (stage == 5) return "Out for a little stroll.";
    if (hour < 12) return "Getting my errands done.";
    if (hour < 17) return "Enjoying some free time.";
    return "Heading home later.";
}
// All coordinates are in the patch's relative world units, not cells.
static inline int ds_cit_in_patch(int x, int y, int span) {
    return x >= -span && y >= -span && x < 2 * span && y < 2 * span;
}
static inline int ds_cit_incoming(int j, int dx, int dy) {
    int x = j % 3 - 1 + dx, y = j / 3 - 1 + dy;
    return x < -1 || x > 1 || y < -1 || y > 1;
}

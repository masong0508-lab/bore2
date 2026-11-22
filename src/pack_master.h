// Pack-goal mastery rules shared by the PS1 pack page and host regression.
// No allocation, extra save bytes, or change to the historical reward bit.
#ifndef BORE2_PACK_MASTER_H
#define BORE2_PACK_MASTER_H

#define PACK_MASTER_FLAG 0x80u
static inline unsigned pack_goal_mask(unsigned count) {
    return count >= 1u && count <= 5u ? (1u << count) - 1u : 0u;
}
// Used *after* claiming available goals: mastery is earned once, in that
// same press. A historical bonus flag is never paid a second time.
static inline int pack_master_due(unsigned flags, unsigned count) {
    unsigned mask = pack_goal_mask(count);
    return mask != 0u && !(flags & PACK_MASTER_FLAG) && (flags & mask) == mask;
}
// The badge describes the current set of goals, not only the historical
// reward flag: old three-goal DOG saves may have the bonus but only 3/5 done.
static inline int pack_master_complete(unsigned flags, unsigned count) {
    unsigned mask = pack_goal_mask(count);
    return mask != 0u && (flags & PACK_MASTER_FLAG) && (flags & mask) == mask;
}
#endif

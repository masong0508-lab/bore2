// Shared, allocation-free Create a Dog goal rules.
// Reused by the life simulation and PKINFO overlay; no new save fields.
#ifndef BORE2_PACK_DOG_PROGRESS_H
#define BORE2_PACK_DOG_PROGRESS_H

static inline unsigned pack_dog_play_next(unsigned count, int pet_kind) {
    return pet_kind == 1 && count < 255u ? count + 1u : count;
}
static inline int pack_dog_play_goal(unsigned count) {
    return count >= 5u;
}
static inline int pack_dog_styled(int pet_kind, int collar, int hat, int outfit) {
    return pet_kind == 1 && (collar != 0 || hat != 0 || outfit != 0);
}
#endif

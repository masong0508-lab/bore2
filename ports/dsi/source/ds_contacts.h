// Fixed-size contacts: preserve the existing serialized CRel layout.
#pragma once
typedef struct { u16 id; u8 rel, talks, taken; } CRel;
#define DS_CONTACT_CAP 64
static inline CRel *ds_contact_find(CRel *table, u16 id) {
    if (!id) return 0;
    unsigned h = id & (DS_CONTACT_CAP - 1);
    for (unsigned k = 0; k < DS_CONTACT_CAP; k++) {
        CRel *c = &table[(h + k) & (DS_CONTACT_CAP - 1)];
        if (c->id == id) return c;
        if (!c->id) return 0;
    }
    return 0;
}
static inline CRel *ds_contact_acquire(CRel *table, u16 id) {
    if (!id) return 0;
    CRel *existing = ds_contact_find(table, id);
    if (existing) return existing;
    unsigned h = id & (DS_CONTACT_CAP - 1);
    CRel *weak = 0;
    for (unsigned k = 0; k < DS_CONTACT_CAP; k++) {
        CRel *c = &table[(h + k) & (DS_CONTACT_CAP - 1)];
        if (!c->id) { weak = c; break; }
        // Never forget a friend, a moved-in townie or a conversation from today.
        if (!c->taken && !c->talks && c->rel < 3 && (!weak || c->rel < weak->rel)) weak = c;
    }
    if (!weak) return 0;
    *weak = (CRel){0}; weak->id = id;
    return weak;
}

/* Keep the saved record layout; the top bit of the daily talk counter is
 * reserved for one phone call, the low bits remain the existing daily limit. */
#define DS_CONTACT_CALLED 0x80
#define DS_CONTACT_TALK_MASK 0x7f
enum { DS_CALL_OK, DS_CALL_UNKNOWN, DS_CALL_NOT_FRIEND, DS_CALL_ALREADY, DS_CALL_BUSY, DS_CALL_CLOSED };
static inline int ds_contact_can_call(const CRel *c, int hour) {
    if (!c || !c->id || c->taken) return DS_CALL_UNKNOWN;
    if (c->rel < 3) return DS_CALL_NOT_FRIEND;
    if (hour < 8 || hour >= 21) return DS_CALL_CLOSED;
    if (c->talks & DS_CONTACT_CALLED) return DS_CALL_ALREADY;
    if ((c->talks & DS_CONTACT_TALK_MASK) >= 4) return DS_CALL_BUSY;
    return DS_CALL_OK;
}
static inline int ds_contact_call(CRel *c, int hour) {
    int result = ds_contact_can_call(c, hour);
    if (result != DS_CALL_OK) return result;
    c->talks = (u8)((c->talks + 1) | DS_CONTACT_CALLED);
    if (c->rel < 10) c->rel++;
    return DS_CALL_OK;
}
static inline int ds_contact_count(const CRel *table, int friends_only) {
    int n = 0;
    for (int i = 0; i < DS_CONTACT_CAP; i++) if (table[i].id && !table[i].taken && table[i].rel >= (friends_only ? 3 : 1)) n++;
    return n;
}
static inline int ds_contact_slot(const CRel *table, int ordinal) {
    if (ordinal < 0) return -1;
    for (int i = 0; i < DS_CONTACT_CAP; i++) if (table[i].id && !table[i].taken && table[i].rel > 0 && ordinal-- == 0) return i;
    return -1;
}

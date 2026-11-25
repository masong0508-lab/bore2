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

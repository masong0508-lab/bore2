#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint16_t u16; typedef uint8_t u8;
#include "../source/ds_contacts.h"
int main(void) {
    CRel table[DS_CONTACT_CAP] = {{0}};
    for (int i = 1; i <= 5000; i++) assert(!ds_contact_find(table, (u16)i));
    for (int i = 0; i < DS_CONTACT_CAP; i++) assert(!table[i].id);
    CRel *a = ds_contact_acquire(table, 1), *b = ds_contact_acquire(table, 65);
    assert(a && b && a != b); a->rel = 7; b->taken = 1;
    for (int i = 2; i < 64; i++) assert(ds_contact_acquire(table, (u16)i));
    assert(ds_contact_acquire(table, 129)); assert(ds_contact_find(table,1)->rel == 7);
    assert(ds_contact_find(table,65)->taken);
    for (int i = 0; i < DS_CONTACT_CAP; i++) { table[i].rel = 3; table[i].talks = 0; }
    CRel before[DS_CONTACT_CAP]; memcpy(before,table,sizeof table);
    assert(!ds_contact_acquire(table, 193)); assert(!memcmp(before,table,sizeof table));
    assert(ds_contact_find(table,1) == a); assert(!ds_contact_acquire(table,0));
    CRel phone[DS_CONTACT_CAP] = {{0}};
    CRel *friend = ds_contact_acquire(phone, 101), *stranger = ds_contact_acquire(phone, 102);
    assert(friend && stranger); friend->rel = 3; stranger->rel = 1;
    assert(ds_contact_count(phone,0)==2 && ds_contact_count(phone,1)==1);
    assert(ds_contact_slot(phone,0)>=0 && ds_contact_slot(phone,1)>=0 && ds_contact_slot(phone,2)<0);
    assert(ds_contact_call(friend,7)==DS_CALL_CLOSED && friend->rel==3 && friend->talks==0);
    assert(ds_contact_call(stranger,12)==DS_CALL_NOT_FRIEND);
    assert(ds_contact_call(friend,12)==DS_CALL_OK && friend->rel==4 && (friend->talks & DS_CONTACT_TALK_MASK)==1);
    assert(ds_contact_call(friend,12)==DS_CALL_ALREADY && friend->rel==4);
    friend->talks=4; assert(ds_contact_call(friend,12)==DS_CALL_BUSY);
    friend->talks=0; assert(ds_contact_call(friend,12)==DS_CALL_OK);
    friend->talks=0; friend->taken=1; assert(ds_contact_call(friend,12)==DS_CALL_UNKNOWN);
    puts("DS contacts: identity, lookup, capacity, protection and daily phone calls passed");
    return 0;
}

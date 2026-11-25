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
    puts("DS contact identity, read-only lookups, full capacity, protected contacts passed");
    return 0;
}

#include <assert.h>
#include <stdio.h>
#include "ds_lot_shell.h"
int main(void){
    BInfo b={.terrain=BT_GRASS,.lot=LOT_HOUSE};LotPlan p={0};DsLotShell s;
    p.face=LF_SOUTH;p.type=HT_L;p.x0=8;p.y0=7;p.x1=14;p.y1=14;p.wx0=5;p.wx1=8;p.wy0=7;p.wy1=12;
    assert(ds_lot_shell(&b,&p,100,100,&s));
    assert(s.body.x0==3&&s.body.x1==10&&s.wing.x0==0&&s.wing.x1==3);
    assert(ds_shell_contains(&s,1,4)&&!ds_shell_contains(&s,1,9));
    assert(ds_shell_contains(&s,3,9)&&!ds_shell_contains(&s,10,9));
    assert(!ds_shell_valid(s.loft));
    p.type=HT_MANSION;p.wx1=0;
    assert(ds_lot_shell(&b,&p,100,100,&s));
    assert(s.loft.z0==2&&s.loft.z1==6);
    p.face=LF_NORTH;
    assert(ds_lot_shell(&b,&p,100,100,&s));
    assert(s.loft.z0==6&&s.loft.z1==10);
    p.type=HT_NONE;
    b.lot=LOT_ARCADE;
    assert(ds_lot_shell(&b,&p,100,100,&s));
    assert(s.body.x0==1&&s.body.z0==4&&s.body.x1==11&&s.body.z1==11);
    b.lot=LOT_POOL;
    assert(ds_lot_shell(&b,&p,100,100,&s));
    assert(s.body.x0==2&&s.body.z0==4&&s.body.z1==12);
    b.lot=LOT_FARM;
    assert(ds_lot_shell(&b,&p,100,100,&s));
    assert(s.body.x0==8&&s.body.z0==6&&s.body.x1==13);
    p.face=LF_NONE;assert(!ds_lot_shell(&b,&p,100,100,&s));
    b.lot=LOT_PARK;assert(!ds_lot_shell(&b,&p,100,100,&s));
    b.lot=LOT_APART;p.face=LF_NORTH;assert(ds_lot_shell(&b,&p,10,20,&s)&&!s.north&&!ds_shell_valid(s.loft));
    b.terrain=BT_SEA;assert(!ds_lot_shell(&b,&p,10,20,&s));
    puts("DS shell: exact L-wing, loft, public hall and north-facing boundaries passed");
}

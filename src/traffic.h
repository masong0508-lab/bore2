#pragma once
// the street traffic of San Boreas, and the vehicles you leave parked
extern int traffic_level;   // the world's dial (CREATE A WORLD): 0 NONE, 1 LOW, 2 NORMAL, 3 BUSY, 4 JAMMED
extern int traffic_cheat;   // the cheat (OPTIONS): extra cars on top, up to a ludicrous number
void traffic_update(int steps);
void traffic_draw(void);
void traffic_clear(void);
void traffic_honk(void);
void traffic_park(int type, int x, int z, int yaw);   // leave a vehicle where it stands
int  traffic_park_near(int type, int x, int z);       // is that vehicle parked within a few tiles of here? (1 and it is picked up)
int traffic_park_pose_near(int type,int x,int z,int *outx,int *outz,int *yaw);
void traffic_unpark(int type);                       // remove a confiscated or newly acquired vehicle's old parking record
int  traffic_parked(int type);                        // is it parked somewhere?
void traffic_save(void);
void traffic_load(int ver);
int  traffic_pcar_tile(int mx, int my);              // (world version 7) 1: a parked car on a home's drive stands on this window tile (it is not walkable)

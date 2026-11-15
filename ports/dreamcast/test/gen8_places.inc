// the places of the world version 8 Dreamcast shots (world 0x5EED1234; -DAUTO_AT_G8=1 -DAUTO_AT_ONE=k holds place k from game frame 0):
// { bx, by, tx, ty, pitch, dist, yaw, dx, dy } - tile 2,2 is the middle of a block's corner square (the turning head, bend corner or junction);
// the blocks are the ones `LISTEND=1 ports/host/townmap/townmap streets 0x5EED1234 8` lists as DEAD, BEND and TEE.
// dx, dy: the tiles from the spot where the sims stand (off the street, or every car would wait for them).
{ 413, 463, 2, 2, 720, 3600, 0, -5, 0 },     // 0 dead end 413,463 (the street arrives from the south): the turning head
{ 414, 465, 2, 2, 720, 3600, 2048, -5, 0 },  // 1 bend 414,465 (streets east and south), camera turned (yaw 2048) so the arms run up and left, clear of the HUD
{ 414, 466, 2, 2, 720, 3600, 0, 5, 0 },      // 2 tee junction 414,466 (streets north, south, west)
{ 409, 467, 2, 2, 720, 3600, 0, 5, 0 },      // 3 tee junction 409,467 (north, south, west)
{ 408, 464, 2, 2, 720, 3600, 0, -5, 0 },     // 4 dead end 408,464 (the street arrives from the east): a head the other way round

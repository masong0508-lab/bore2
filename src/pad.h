// pad: the controller, read directly once a frame (see pad.c)
#pragma once
void pad_init(void);
int  pad_read(int port, u8 *out);   // out[8]: 0 stat, 1 id (0x41 digital, 0x73 analog), 2-3 buttons (0 = pressed), 4-7 sticks

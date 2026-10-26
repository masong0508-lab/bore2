// pad: the controller, read directly once a frame (see pad.c)
#pragma once
void pad_init(void);
int  pad_read(int port, u8 *out);   // out[8]: 0 stat, 1 id (0x41 digital, 0x73 analog), 2-3 buttons (0 = pressed), 4-7 sticks
// the serial port itself, for the memory card (mcard.c): select a port, swap bytes, let go
void sio_select(int port);
int  sio_xfer(int byte, int want_ack, int wait);   // the reply, or -1
void sio_deselect(void);
void pad_poll(void);              // one extra read: buttons seen down are kept for the next frame (see pad.c)
unsigned short pad_latch_take(void);   // the buttons seen down since the last take (active low), and start afresh
void pad_poll_on(int on);

// pad: the controller, read directly from the PlayStation's serial port once a frame (no BIOS pad driver).
// The BIOS driver polls the pad inside its own vblank interrupt handling; when the vblank interrupt lands while another
// interrupt (the music timer) is being handled, the SDK's handler acknowledges it first and the BIOS never polls - for
// as long as the two stay in step, the pad goes dead. Reading it here, in the main loop, cannot miss.
#include "common.h"
#include <hwregs_c.h>
#include "pad.h"

static void spin(int n) { for (volatile int i = 0; i < n; i++) { } }
int sio_xfer(int byte, int want_ack, int wait) {   // send a byte, return the reply (-1: no reply / no /ACK within wait spins)
    SIO_DATA(0) = (u8)byte;
    int t = 0;
    while (!(SIO_STAT(0) & 0x0002)) if (++t > wait) return -1;   // the reply comes back as the byte goes out
    int r = SIO_DATA(0);
    if (want_ack) {                                  // a device that wants more pulses /ACK: with the /ACK interrupt enabled, that sets STAT bit 9
        t = 0; while (!(SIO_STAT(0) & 0x0200)) if (++t > wait) return -1;
    }
    SIO_CTRL(0) |= 0x0010;                           // acknowledge it (the CPU never sees the interrupt: IRQ 7 stays masked)
    return r & 255;
}
static int xfer(int byte, int want_ack) { return sio_xfer(byte, want_ack, 3000); }
void sio_select(int port) {                          // /CS down on that port (the pad and the memory card share it: the first byte says which)
    SIO_CTRL(0) = 0x1003 | (port << 13);             // TX on, select the port, /ACK raises STAT bit 9
    spin(60);                                        // (/CS low ~20 us before the first byte)
    while (SIO_STAT(0) & 0x0002) (void)SIO_DATA(0);  // flush
}
void sio_deselect(void) { SIO_CTRL(0) = 0; }
void pad_init(void) {
    SIO_CTRL(0) = 0x0040;                            // reset the port
    SIO_BAUD(0) = 0x0088;                            // 250 kHz
    SIO_MODE(0) = 0x000d;                            // 8 bits, no parity, x1
    SIO_CTRL(0) = 0;
}
int pad_read(int port, u8 *out) {                    // out: the BIOS layout (stat, id, buttons, sticks); 0 = nothing there
    memset(out, 0, 8); out[0] = 0xff;
    sio_select(port);
    int ok = xfer(0x01, 1) >= 0;                     // address the controller
    int id = ok ? xfer(0x42, 1) : -1;                // read: the reply is its type
    int n = 0;
    if (id >= 0 && xfer(0x00, 1) == 0x5a) {
        int words = id & 15; if (!words) words = 16;
        int bytes = MIN(words * 2, 6);
        for (n = 0; n < bytes; n++) { int v = xfer(0x00, n < bytes - 1); if (v < 0) break; out[2 + n] = (u8)v; }
    }
    SIO_CTRL(0) = 0;                                 // deselect
    if (n < 2) return 0;
    out[0] = 0; out[1] = (u8)id;
    return 1;
}

// ---- taps between frames: the pad is also polled while the game waits for the next vblank (the halt function of VSync) and a
// few times in a frame; every button seen down at any poll counts as held for the frame that follows (a quick tap can no longer
// fall between two reads). Buttons are active low, so the latch is an AND.
static u16 latch = 0xffff; static int last_poll_hb = -1000, pad_live;
void pad_poll(void) {
    if (!pad_live) return;
    int hb = VSync(1); if (hb - last_poll_hb >= 0 && hb - last_poll_hb < 12) return;   // (not more than one a ~0.75 ms)
    last_poll_hb = hb;
    u8 b[8]; if (!pad_read(0, b) || b[0] != 0) return;
    if (b[1] != 0x41 && b[1] != 0x73 && b[1] != 0x53 && b[1] != 0x23) { int t = b[1] >> 4; if (t != 4 && t != 5 && t != 7) return; }
    latch &= (u16)(b[2] | (b[3] << 8));
}
u16 pad_latch_take(void) { u16 l = latch; latch = 0xffff; return l; }
void pad_poll_on(int on) { pad_live = on; if (on) VSyncHaltFunction(pad_poll); }

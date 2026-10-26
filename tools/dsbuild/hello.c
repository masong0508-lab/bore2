#include <nds.h>
#include <stdio.h>
int main(void) {
    consoleDemoInit();
    iprintf("BORE 2 DSI BUILD TEST\n");
    int n = 0; while (1) { swiWaitForVBlank(); iprintf("frame %d\r", n++); if (n == 120) iprintf("\nDONE\n"); }
}

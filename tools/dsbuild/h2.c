#include <nds.h>
int main(void) {
    powerOn(POWER_ALL_2D);
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    for (int i = 0; i < 256*192; i++) VRAM_A[i] = RGB15(i&31, (i>>8)&31, 31) | BIT(15);
    while (1) swiWaitForVBlank();
}

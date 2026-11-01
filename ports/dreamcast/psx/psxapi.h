/* Dreamcast stand-in for PSn00bSDK's psxapi.h: only what BORE 2 calls (the BIOS file and event calls clash with the C library). */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <hwregs_c.h>
#define FastEnterCriticalSection() uint16_t __saved_irq_mask = IRQ_MASK; (IRQ_MASK = 0)
#define FastExitCriticalSection() (IRQ_MASK = __saved_irq_mask)
#ifdef __cplusplus
extern "C" {
#endif
int EnterCriticalSection(void);
void ExitCriticalSection(void);
#ifdef __cplusplus
}
#endif

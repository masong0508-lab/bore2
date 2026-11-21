#ifndef BORE2_DS_CONFIRM_H
#define BORE2_DS_CONFIRM_H
/* Deliberate two-tap confirmation for destructive UI commands. VBlank
 * subtraction remains correct even when the unsigned frame counter wraps.
 * No save-file fields, allocations, or platform SDK dependencies. */
static int ds_confirm_expired(unsigned int now, unsigned int stamp,
                              unsigned int window)
{
    return (unsigned int)(now - stamp) > window;
}
static int ds_confirm_twice(unsigned int now, unsigned int *stamp,
                            unsigned char *armed, unsigned int window)
{
    if (*armed && !ds_confirm_expired(now, *stamp, window)) {
        *armed = 0;
        return 1;
    }
    *stamp = now;
    *armed = 1;
    return 0;
}
#endif

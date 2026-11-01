// force-included before every shared game source (src/*.c) in the Dreamcast build: the C library headers first, then the renames
// of the game's names that the library also declares (so src/ needs no change).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#define link bore_link

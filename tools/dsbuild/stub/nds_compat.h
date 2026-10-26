#pragma once
// libnds names newlib's integer-only printf family; here they are the ordinary ones
#include <stdio.h>
#define iprintf printf
#define siprintf sprintf
#define sniprintf snprintf
#define viprintf vprintf
#define siscanf sscanf

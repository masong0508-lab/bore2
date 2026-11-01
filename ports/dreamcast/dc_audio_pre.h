// Force-included before src/audio.c only (with dc_shared_pre.h): the key registers become events for the software SPU, and the
// sound code's public calls are renamed so that dc_spu.c can take each under the mixer's lock (dc_audio_names.h is generated).
#define DC_SPU_EVENTS 1
#include "dc_audio_names.h"

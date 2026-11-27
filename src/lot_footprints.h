#pragma once
#include "bigmap.h"
/* The non-residential building footprint in south-facing plot coordinates.
 * The console generator, DS public venues and map models share this table. */
static const u8 bore_lot_footprints[LOT_N][4] = {
    [LOT_SHOP] = { 1, 1, 11, 9 }, [LOT_DINER] = { 1, 2, 11, 9 }, [LOT_GYM] = { 1, 2, 11, 9 }, [LOT_OFFICE] = { 1, 2, 11, 9 }, [LOT_INN] = { 0, 1, 12, 11 },
    [LOT_SCHOOL] = { 0, 1, 12, 8 }, [LOT_ARCADE] = { 1, 2, 11, 9 }, [LOT_TOWER] = { 1, 1, 11, 10 }, [LOT_HOTEL] = { 0, 0, 12, 12 }, [LOT_APART] = { 0, 1, 12, 10 },
    [LOT_CAFE] = { 1, 2, 11, 9 }, [LOT_BAR] = { 1, 2, 11, 9 }, [LOT_CINEMA] = { 0, 0, 12, 11 }, [LOT_SALON] = { 1, 2, 11, 9 }, [LOT_RESTAURANT] = { 0, 1, 12, 10 },
    [LOT_CLINIC] = { 1, 1, 11, 10 }, [LOT_LIBRARY] = { 0, 1, 12, 10 }, [LOT_SPA] = { 1, 1, 11, 10 }, [LOT_GALLERY] = { 1, 2, 11, 9 }, [LOT_GAS] = { 1, 1, 11, 5 },
    [LOT_CONV] = { 2, 3, 10, 9 }, [LOT_FARM] = { 8, 1, 12, 6 }, [LOT_SPACE] = { 1, 1, 11, 6 }, [LOT_OBS] = { 1, 2, 11, 9 } };

static inline int bore_lot_has_footprint(int lot){
    return (unsigned)lot<LOT_N&&bore_lot_footprints[lot][2]!=0;
}

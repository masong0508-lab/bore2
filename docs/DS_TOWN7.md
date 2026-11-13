# DS world version 7 (d79)

Note: DS worlds before d79 were world version 3 (bm_gen was never set), not 6.

What it does
- bm_gen = 7 at boot, new world and new life; save opt[38] = 7 marks a gen 7 life. Old saves (opt[38] = 0) load as gen 3 and keep their towns.
- src/bigmap.c unchanged. ports/dsi/source/ds_town7.h holds the DS side: plan homes from LotPlan (ranch, L with garage wing, box, two-storey with loft, cottage, terrace pair, mansion), yards, drives, front hedge row, mailbox, street trees, back-yard patio/bbq/beds/swing, rooms (kitD / kitR / kitC: bedroom + bath, living group, kitchen + dining), north-facing lots built south then flipped (flip7), far view footprints from the plan (far_foot7, far_lot).
- Test hooks: -DDS_AT_BX/BY start lot; AUTO_SCENE 171-175 camera shots.

Measured (melonDS, tools/dsemu/tb.sh): avg 27.8 fps (26-30), quads 2891-3119; d78 baseline 29.6 fps, 2973-3201 quads. Not yet back at 29-30.

Not done: in-ROM overlap/reachability audit of every house type, Map lot picture from the plan, save round trip test in melonDS, ds_oldsave/ds_town runs, shots of a suburb street, town centre and interior, fps tuning.

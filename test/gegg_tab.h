// the egg shots (test/gegg.sh = test/gseq.sh with this table, a build with -DAUTO_GOTCHI=0 -DAUTO_GSEQ -DAUTO_GEGG): {view, sub, clock, x, a} and a label after //.
// x: 128 = every slot an egg (slot i pattern i, slot 6 a rare one), bits 0-3 = the slot to show + 1 (8: slot 7, a mystery egg);  a: that egg's age + 1 (15 on is cracking).  sub is the guide page for V_GUIDE.
{V_INTRO,0,60}, // shop card
{V_INTRO,1,150}, // unbox reveal t150
{V_INTRO,2,60}, // egg card
{V_HAND,0,0}, // day one card
{V_HAND,7,0,0x81}, // pet view pattern 0
{V_HAND,7,0,0x82}, // pet view pattern 1
{V_HAND,7,0,0x83}, // pet view pattern 2
{V_HAND,7,0,0x84}, // pet view pattern 3
{V_HAND,7,0,0x85}, // pet view pattern 4
{V_HAND,7,0,0x86}, // pet view pattern 5
{V_HAND,7,0,0x87}, // pet view rare egg
{V_HAND,7,0,0x83,16}, // pet view crack 1 (age 15)
{V_HAND,7,0,0x83,18}, // pet view crack 2 (age 17)
{V_HAND,7,0,0x83,20}, // pet view crack 3 (age 19)
{V_BASKET,0,0,0x87}, // basket
{V_HOME,0,0,0x81}, // family home
{V_TREE,0,0,0x83}, // family tree
{V_DAY,0,0,0x81}, // day card tiles
{V_GUIDE,2,0,0x81}, // guide page 3
{V_GUIDE,3,0,0x81}, // guide page 4
{V_HATCH,0,0,0x86}, // hatch t0 pattern 5
{V_HATCH,0,14,0x86}, // hatch t14
{V_HATCH,0,38,0x86}, // hatch t38 wobble
{V_HATCH,0,56,0x86}, // hatch t56 crack 1
{V_HATCH,0,66,0x86}, // hatch t66 crack 2
{V_HATCH,0,76,0x86}, // hatch t76 crack 3
{V_HATCH,0,86,0x86}, // hatch t86 flash
{V_HATCH,0,90,0x86}, // hatch t90 pop
{V_HATCH,0,96,0x86}, // hatch t96 shell and baby
{V_HATCH,0,104,0x86}, // hatch t104
{V_HATCH,0,0,0x82}, // hatch t0 pattern 1
{V_HATCH,0,0,0x84}, // hatch t0 pattern 3
{V_HATCH,0,0,0x87}, // hatch t0 rare egg
{V_HATCH,0,76,0x87}, // hatch t76 rare egg
{V_INTRO,0,60,0x88}, // shop card, mystery egg
{V_HAND,7,0,0x88}, // pet view mystery egg
{V_HAND,7,0,0x88,16}, // mystery egg crack 1 (age 15)
{V_HAND,7,0,0x88,20}, // mystery egg crack 3 (age 19)
{V_BASKET,0,0,0x88}, // basket with the mystery egg
{V_HOME,0,0,0x88}, // home with the mystery egg
{V_TREE,0,0,0x88}, // tree, mystery egg badge
{V_DIARY,0,0,0x88}, // diary: hatched, egg, mystery egg
{V_HATCH,0,0,0x88}, // hatch t0 mystery egg
{V_HATCH,0,40,0x88}, // hatch t40 mystery egg
{V_HATCH,0,76,0x88}, // hatch t76 mystery egg
{V_HATCH,0,120,0x88}, // hatch t120 from a mystery egg
{V_BOX,0,0,0x88}, // toy box: pets, the mystery egg in amber

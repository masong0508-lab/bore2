#!/bin/bash
# gegg.sh TAG: shots of every Boreagotchi egg (pet view, basket, home, tree, day card, guide, toy LCD cards, hatching) into docs/gotchi_egg_TAG_N.png (sheets of 12).
# A build with -DAUTO_GOTCHI=0 -DAUTO_GSEQ -DAUTO_GEGG in $GV (default /tmp/c_gegg); the steps are test/gegg_tab.h. Same scratch folders as gseq.sh (DSD DSR DSL, GVN).
cd "$(dirname "$0")/.."
GV=${GV:-/tmp/c_gegg} GVN=${GVN:-gegg} PFX=gotchi_egg TABF=test/gegg_tab.h exec bash test/gseq.sh "${1:-after}"

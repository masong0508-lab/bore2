#!/bin/bash
# shot.sh rom frames outbase "script" "f1,f2,..." -> outbase.png and outbase_N.png (top screen over bottom screen)
# script and frames as in run.cpp; DBGMEM etc. pass through. Needs python3 + PIL.
rm -f "$3"*.ppm
${DSRUN:-/tmp/dsrun} "$1" "$2" "$3.ppm" "$4" "$5" 2>&1 | tail -60
python3 - "$3" <<'PY'
import sys, glob
from PIL import Image
b = sys.argv[1]
for f in glob.glob(b + "*.ppm"):
    Image.open(f).save(f[:-4] + ".png")
PY
rm -f "$3"*.ppm

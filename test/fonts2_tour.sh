#!/bin/bash
# fonts2_tour.sh BUILDDIR NAME [frames]: runs BUILDDIR/bore2_lite.cue (a Release build with -DAUTO_SCREENS=420 -DCIT_KNOWN=3, ninja iso_lite) through the text tour
# (python3 -I test/fonts2_tour.py gen writes the pad script); the frames land in /tmp/f2/dsd/NAME/ in the order of /tmp/f2/tour.json.
# Then: python3 -I test/fonts2_tour.py pair BEFOREname AFTERname  writes docs/fonts2_*.png.   (DS8MB=1 in the environment boots on 8 MB of RAM.)
HERE=$(cd "$(dirname "$0")" && pwd)
export DSD=${DSD:-/tmp/f2/dsd} DSR=${DSR:-/tmp/f2/dsr} DSL=${DSL:-/tmp/f2}
F=${3:-$(python3 -I -c "import json;print(json.load(open('/tmp/f2/tour.json'))['frames'])")}
bash "$HERE/ds.sh" "$1/bore2_lite.cue" "$2" "$F" /tmp/f2/tour.txt
ls "$DSD/$2" | wc -l

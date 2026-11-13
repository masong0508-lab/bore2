#!/bin/bash
# views.sh BUILDDIR NAME [PLAN.h]: the 3D audit run. BUILDDIR holds a Release build with -DAUTO_PHONE=1 -DAUTO_VIEWS=1 (ninja iso).
# Boots it in DuckStation (test/ds.sh), takes the shots views_script.py lists from the plan, writes frames to $DSD/NAME/ and a contact sheet $DSD/NAME_sheet.png (4 across).
# Env: DSD DSR DSL as test/ds.sh; T0 = the frame the life starts at (default 1950).
B=$1; N=$2; P=${3:-$(dirname $0)/views_plan.h}
DSL=${DSL:-/tmp}; export DSD=${DSD:-/tmp/dsd} DSR=${DSR:-/tmp/dsr} DSL
python3 -I $(dirname $0)/views_script.py $P $DSL/$N.txt ${T0:-1950} | tee $DSL/$N.info
LAST=$(sed 's/.*last frame //' $DSL/$N.info)
bash $(dirname $0)/ds.sh $B/bore2.cue $N $((LAST + 400)) $DSL/$N.txt
python3 -I - <<EOF
from PIL import Image
import glob
fs = sorted(glob.glob('$DSD/$N/frame_*.png'))
cols = 4; rows = (len(fs) + cols - 1) // cols
c = Image.new('RGB', (292 * cols, 224 * rows))
for i, f in enumerate(fs): c.paste(Image.open(f).convert('RGB'), ((i % cols) * 292, (i // cols) * 224))
c.save('$DSD/${N}_sheet.png')
print(len(fs), 'frames ->', '$DSD/${N}_sheet.png')
EOF

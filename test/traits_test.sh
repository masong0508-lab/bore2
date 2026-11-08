#!/bin/sh
# traits_test.sh: make the data, build the host test against src/traits.c, run it, then check every hook is read somewhere in the game.
HERE=$(cd "$(dirname "$0")" && pwd); R="$HERE/.."
python3 "$R/tools/make_traits.py" > /dev/null || exit 1
gcc -O1 -Wall -DTR_HOST -DTR_BIG -o /tmp/traits_test "$HERE/traits_test.c" "$R/src/traits.c" "$R/src/traits_auto.c" "$R/src/persona.c" || exit 1
/tmp/traits_test; RC=$?
echo; echo "== is each hook read by the game? (a hook name used in src outside traits*, or its family base)"
MISS=0
for h in $(grep -o '^    E_[A-Z_0-9]*' "$R/src/traits_data.h" | tr -d ' ' | grep -v '^E_N$\|^E_NONE$'); do
  fam=$h
  case $h in E_DEC_*) fam=E_DEC0;; E_GAIN_*) fam=E_GAIN0;; E_SK_*) fam=E_SK0;; E_PAY_*) fam=E_PAY0;; esac
  if ! grep -q -w -e "$h" -e "$fam" $(ls "$R"/src/*.c "$R"/src/*.inc "$R"/src/*.h | grep -v 'traits_data.h\|/traits.h\|/traits.c\|traits_ui.c\|traits_auto.c') 2>/dev/null; then echo "  NOT WIRED: $h"; MISS=$((MISS+1)); fi
done
echo "hooks not wired: $MISS"
exit $((RC + MISS))

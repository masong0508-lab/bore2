# The town-plan tool (see townmap.c). `make` builds ./townmap. `make check` proves the worlds of before the plan (versions 2 to 6)
# still come out block for block as bigmap.c made them at OLD_REV. `make pics` writes docs/plan7_*.png (needs python3 with Pillow).
CC ?= gcc
CFLAGS = -O1 -g -I../shim -I../../../src -w -std=gnu11
OLD_REV ?= 0fdbfe3
SRC = ../../../src
all: townmap
townmap: townmap.c $(SRC)/bigmap.c $(SRC)/bigmap.h
	$(CC) $(CFLAGS) -o $@ townmap.c $(SRC)/bigmap.c
old/bigmap.c:
	mkdir -p old && git show $(OLD_REV):src/bigmap.c > old/bigmap.c && git show $(OLD_REV):src/bigmap.h > old/bigmap.h
townmap_old: townmap.c old/bigmap.c
	$(CC) -O1 -g -Iold -I../shim -I../../../src -w -std=gnu11 -DTOWNMAP_OLD -o $@ townmap.c old/bigmap.c
check: townmap townmap_old
	@fail=0; for seed in 1234 0x5EED1234 777 99; do for gen in 2 3 4 5 6; do for pk in roomy packed; do \
	  a=`./townmap sum $$seed $$gen $$pk`; b=`./townmap_old sum $$seed $$gen $$pk`; \
	  if [ "$$a" != "$$b" ]; then echo "DIFFERENT: seed $$seed version $$gen $$pk: $$a / $$b"; fail=1; fi; done; done; done; \
	  if [ $$fail = 0 ]; then echo "check: the worlds of versions 2 to 6 are the same as before the plan"; else exit 1; fi
pics: townmap
	python3 mkpics.py
clean:
	rm -rf townmap townmap_old old

# The lot simulator (see lotsim.c). `make` builds ./lotsim (the generators in src/lots.inc) and ./lotsim_old (the generators as they were at OLD_REV, before the lots round).
CC ?= gcc
CFLAGS = -O1 -g -I../shim -I../../../src -I. -w -std=gnu11
OLD_REV ?= c2408e4
SRC = ../../../src
all: lotsim lotsim_old
items_tab.h: mkitems.py $(SRC)/items.c
	python3 mkitems.py items_tab.h
old_gen.inc:
	( git show $(OLD_REV):src/world.c > old_world.c.tmp && ( sed -n 575,1262p old_world.c.tmp; sed -n 1309,1409p old_world.c.tmp ) > old_gen.inc; rm -f old_world.c.tmp )
lotsim: lotsim.c items_tab.h $(SRC)/lots.inc $(SRC)/lots7.inc $(SRC)/lot_audit.inc $(SRC)/bigmap.c
	$(CC) $(CFLAGS) -o $@ lotsim.c $(SRC)/bigmap.c
lotsim_old: lotsim.c items_tab.h old_gen.inc $(SRC)/lot_audit.inc $(SRC)/bigmap.c
	$(CC) $(CFLAGS) -DLOTSIM_OLD -o $@ lotsim.c $(SRC)/bigmap.c
clean:
	rm -f lotsim lotsim_old items_tab.h old_gen.inc

# The traffic audit (see trafsim.c). `make` builds ./trafsim from the game's own src/traffic.c; `make old TRAF_OLD=path` builds ./trafsim_old from another traffic.c to compare.
CC ?= gcc
CFLAGS = -O1 -g -I../shim -I../../../src -w -std=gnu11 -DTRAF_DBG=1
SRC = ../../../src
all: trafsim
trafsim: trafsim.c $(SRC)/traffic.c $(SRC)/traffic.h $(SRC)/vehicle_stability.h $(SRC)/bigmap.c $(SRC)/platform/compat_math.c
	$(CC) $(CFLAGS) -o $@ trafsim.c $(SRC)/bigmap.c $(SRC)/platform/compat_math.c -lm
old: trafsim.c $(SRC)/bigmap.c $(SRC)/platform/compat_math.c
	$(CC) $(CFLAGS) -DTRAF_SRC='"$(TRAF_OLD)"' -o trafsim_old trafsim.c weak.c $(SRC)/bigmap.c $(SRC)/platform/compat_math.c -lm
clean:
	rm -f trafsim trafsim_old

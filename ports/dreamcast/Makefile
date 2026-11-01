# BORE 2 on the Dreamcast (KallistiOS). Build:  source /tmp/dc/environ.sh && make     (see README.md for building the toolchain)
# The shared game code in ../../src is compiled unchanged; only the files in this directory are Dreamcast-specific.
TARGET = bore2_dc.elf
SRC = ../../src
OBJS = dc_main.o dc_gfx.o dc_life.o dc_stubs.o bigmap.o
OBJS += $(EXTRA_OBJS)
KOS_CFLAGS += -DBORE_DC -I../host/shim -I$(SRC) -I../../assets -w -std=gnu11
vpath %.c $(SRC)
# the game's names that the C library also uses (link, ...): renamed for the shared files only, so src/ stays untouched
SHARED_OBJS = bigmap.o $(EXTRA_OBJS)
$(SHARED_OBJS): KOS_CFLAGS += -include dc_shared_pre.h

all: $(TARGET)
include $(KOS_BASE)/Makefile.rules
clean:
	-rm -f $(OBJS) $(TARGET)
$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lm
run: $(TARGET)
	/tmp/dc/dcrun /tmp/dc/build/flycast/flycast_libretro.so $(TARGET) 300 /tmp/dc/shot

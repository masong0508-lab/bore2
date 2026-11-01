# BORE 2 on the Dreamcast (KallistiOS): the PS1 game's own code (../../src, the same file list as CMakeLists.txt, overlays linked in)
# on a Dreamcast platform layer (dc_*.c, psx/*.h). Build:  source /tmp/dc/environ.sh && make
TARGET = bore2_dc.elf
SRC = ../../src
GAME = main gfx atlas look voxel items world sim wants citizens font fmt save crowd bigmap pop far ui places home island nature \
       story skate traffic gotchi band dog ghost realm_res arcade_res
OVLS = realm arcade fight gotchi_ui party_ui dj_ui super_ui dog_ui prison_ui heist_ui space_ui paradise_ui farm_ui skysea_ui cutscene
PLAT = dc_psx dc_gpu dc_plat
OBJS = $(addsuffix .o,$(GAME) $(OVLS) $(PLAT))
KOS_CFLAGS += -DBORE_DC -Ipsx -I$(SRC) -I../../assets -w -std=gnu11 \
              -Wno-error=implicit-function-declaration -Wno-error=int-conversion -Wno-error=incompatible-pointer-types -fno-strict-aliasing
vpath %.c $(SRC)
GAME_OBJS = $(addsuffix .o,$(GAME) $(OVLS))
$(GAME_OBJS): KOS_CFLAGS += -include dc_shared_pre.h
# each overlay's table has the same name on the PS1 (it is its own file there): here they are told apart
realm.o: KOS_CFLAGS += -Dovl_head=ovl_head_kingdom
arcade.o: KOS_CFLAGS += -Dovl_head=ovl_head_arcade
gotchi_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_gotchi
party_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_party
dj_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_dj
super_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_super
dog_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_dog
prison_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_prison
space_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_space
paradise_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_paradise
farm_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_farm
skysea_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_skysea
cutscene.o: KOS_CFLAGS += -Dovl_head=ovl_head_cut

all: $(TARGET)
include $(KOS_BASE)/Makefile.rules
clean:
	-rm -f $(OBJS) $(TARGET)
$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lm

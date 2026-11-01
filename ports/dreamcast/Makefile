# BORE 2 on the Dreamcast (KallistiOS): the PS1 game's own code (../../src, the same file list as CMakeLists.txt, overlays linked in)
# on a Dreamcast platform layer (dc_*.c, psx/*.h). Build:  source /tmp/dc/environ.sh && make
#   make O=obj_city AUTO="AUTO_CITY=1 -DAB_LOCK4=1"   a test build in its own folder (the PS1 build's own test hooks)
#   make disc                                          bore2_dc.gdi: a disc image (the ELF, the songs and voices) for flycast or a GD emulator
O ?= obj
TARGET = $(O)/bore2_dc.elf
SRC = ../../src
GAME = main gfx atlas look voxel items world sim wants citizens font fmt save crowd bigmap pop far ui places home island nature \
       story skate traffic gotchi band dog ghost realm_res arcade_res audio
OVLS = realm arcade fight gotchi_ui party_ui dj_ui super_ui dog_ui prison_ui heist_ui space_ui paradise_ui farm_ui skysea_ui cutscene
PLAT = dc_psx dc_gpu dc_plat dc_spu dc_cd
OBJS = $(addprefix $(O)/,$(addsuffix .o,$(GAME) $(OVLS) $(PLAT)))
KOS_CFLAGS += -DBORE_DC -Ipsx -I. -I$(SRC) -I../../assets -w -std=gnu11 -MMD \
              -Wno-error=implicit-function-declaration -Wno-error=int-conversion -Wno-error=incompatible-pointer-types -fno-strict-aliasing
KOS_CFLAGS += $(if $(AUTO),-D$(AUTO))
vpath %.c $(SRC)
GAME_OBJS = $(addprefix $(O)/,$(addsuffix .o,$(GAME) $(OVLS)))
$(GAME_OBJS): KOS_CFLAGS += -include dc_shared_pre.h
# the PS1 sound code drives the SPU's key registers by writing them: here each write is an event for the software SPU (dc_spu.c)
$(O)/audio.o: KOS_CFLAGS += -include dc_audio_pre.h
# each overlay's table has the same name on the PS1 (it is its own file there): here they are told apart
$(O)/realm.o: KOS_CFLAGS += -Dovl_head=ovl_head_kingdom
$(O)/arcade.o: KOS_CFLAGS += -Dovl_head=ovl_head_arcade
$(O)/gotchi_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_gotchi
$(O)/party_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_party
$(O)/dj_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_dj
$(O)/super_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_super
$(O)/dog_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_dog
$(O)/prison_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_prison
$(O)/space_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_space
$(O)/paradise_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_paradise
$(O)/farm_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_farm
$(O)/skysea_ui.o: KOS_CFLAGS += -Dovl_head=ovl_head_skysea
$(O)/cutscene.o: KOS_CFLAGS += -Dovl_head=ovl_head_cut

all: $(TARGET)
include $(KOS_BASE)/Makefile.rules
$(O)/%.o: %.c | $(O)
	kos-cc $(CFLAGS) $(CPPFLAGS) -c $< -o $@
$(O):
	mkdir -p $(O)
clean:
	-rm -rf $(O)
$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS) -lm
-include $(wildcard $(O)/*.d)

# the disc: IP.BIN (KOS makeip), 1ST_READ.BIN (the ELF as a plain binary, not scrambled: a GD-ROM image), the PS1 game's sound files
disc: $(O)/bore2_dc.gdi
$(O)/bore2_dc.gdi: $(TARGET) ../../assets/SONGS.BIN ../../assets/voices.bin ../../assets/GOTCHI.ART mkdisc.py
	sh-elf-objcopy -O binary -R .stack $(TARGET) $(O)/1ST_READ.BIN
	$(KOS_BASE)/utils/makeip/makeip -f -g "BORE 2" -c "BORE 2 TEAM" $(O)/IP.BIN > /dev/null
	python3 mkdisc.py $(O) $(O)/IP.BIN 1ST_READ.BIN=$(O)/1ST_READ.BIN SONGS.BIN=../../assets/SONGS.BIN VOICES.BIN=../../assets/voices.bin GOTCHI.ART=../../assets/GOTCHI.ART

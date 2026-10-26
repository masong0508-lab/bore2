# BORE 2 for the Nintendo DSi (DSi hardware only). Build with devkitPro: devkitARM + libnds installed ($DEVKITPRO, $DEVKITARM set).
#   make            -> bore2_dsi.nds
# (Not built or run yet by the assistant: there was no DS toolchain or emulator in its workspace. See README.md.)
.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error "Set DEVKITARM (install devkitPro and devkitARM)")
endif
include $(DEVKITARM)/ds_rules

TARGET   := bore2_dsi
BUILD    := build
SOURCES  := source
INCLUDES := source ../../src
ARCH     := -march=armv5te -mtune=arm946e-s -mthumb -mthumb-interwork
CFLAGS   := -g -Wall -O2 $(ARCH) -fomit-frame-pointer -ffast-math -DARM9 $(INCLUDE)
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS  := -g $(ARCH)
LDFLAGS   = -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS     := -lnds9
LIBDIRS  := $(LIBNDS)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT  := $(CURDIR)/$(TARGET)
export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export OFILES   := $(CFILES:.c=.o)
export INCLUDE  := $(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export LD       := $(CC)
.PHONY: $(BUILD) clean
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds
else
DEPENDS := $(OFILES:.o=.d)
$(OUTPUT).nds: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
-include $(DEPENDS)
endif

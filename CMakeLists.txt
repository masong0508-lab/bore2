# BORE 2 (PlayStation) - built with PSn00bSDK
cmake_minimum_required(VERSION 3.21)

project(
	BORE2
	LANGUAGES    C CXX ASM
	VERSION      0.1.0
	DESCRIPTION  "BORE 2: the voxel life, in full 3D"
)

psn00bsdk_add_executable(bore2 GPREL
	src/main.c src/gfx.c src/atlas.c src/look.c src/voxel.c src/items.c src/world.c src/sim.c src/wants.c src/citizens.c src/audio.c src/font.c src/fmt.c src/pad.c src/mcard.c src/save.c src/crowd.c src/mem8.c src/bigmap.c src/pop.c src/far.c src/ui.c src/places.c src/home.c src/island.c src/nature.c src/story.c src/skate.c src/ghost.c src/ovl.c src/realm_res.c src/arcade_res.c ${CMAKE_BINARY_DIR}/ovl_keep.c)
target_include_directories(bore2 PRIVATE src assets)
if(NOT BORE_OPT)
	set(BORE_OPT -O2)
endif()
target_compile_options(bore2 PRIVATE -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-misleading-indentation -Wno-parentheses -Wno-strict-aliasing -fno-strict-aliasing)
get_target_property(BORE_SRCS bore2 SOURCES)
# the life's rules and the menus run a few times a frame at most: built small (-Os), which leaves more of the 2 MB for the world
set(BORE_SMALL src/main.c src/pop.c src/wants.c src/places.c src/look.c src/audio.c src/home.c src/island.c src/nature.c src/save.c src/mcard.c src/sim.c src/story.c src/ui.c src/bigmap.c src/skate.c src/fmt.c src/font.c src/ovl.c src/realm_res.c src/arcade_res.c ${BORE_SMALL_EXTRA})
foreach(f ${BORE_SRCS})
	if(f MATCHES "\\.c$")
		if(f IN_LIST BORE_SMALL)
			set_source_files_properties(${f} PROPERTIES COMPILE_OPTIONS "-Os")
		else()
			set_source_files_properties(${f} PROPERTIES COMPILE_OPTIONS "${BORE_OPT}")   # (after the SDK's own -O2, so it is the one that counts)
		endif()
	endif()
endforeach()

target_link_options(bore2 PRIVATE -Wl,--wrap=memcpy -Wl,--wrap=memmove)   # (src/mem8.c: word copies, not the SDK's byte loops)

# THE OVERLAYS (src/ovl.h): KINGDOM MODE and the arcade are files on the disc, read into the crowd's arena when you walk into them.
# Their code is built on its own (no gp: it is not linked with the game), then linked against bore2.elf's symbols at the arena's address.
foreach(o kingdom arcade)
	if(o STREQUAL kingdom)
		add_library(ovl_${o} OBJECT src/realm.c)
	else()
		add_library(ovl_${o} OBJECT src/arcade.c src/fight.c)
	endif()
	target_include_directories(ovl_${o} PRIVATE src assets)
	target_compile_options(ovl_${o} PRIVATE -Os -G0 -mno-gpopt -mlong-calls -fno-pic -mno-abicalls -ffunction-sections -fdata-sections -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-misleading-indentation -Wno-parentheses -fno-strict-aliasing)
endforeach()
# what the overlays use from the game: a table of it, so the game's linker keeps it
add_custom_command(
	OUTPUT  ${CMAKE_BINARY_DIR}/ovl_keep.c
	COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/ovl.py keep ${CMAKE_BINARY_DIR}/ovl_keep.c ${TOOLCHAIN_NM} $<TARGET_OBJECTS:ovl_kingdom> $<TARGET_OBJECTS:ovl_arcade>
	DEPENDS ovl_kingdom ovl_arcade $<TARGET_OBJECTS:ovl_kingdom> $<TARGET_OBJECTS:ovl_arcade> ${CMAKE_SOURCE_DIR}/tools/ovl.py
	COMMAND_EXPAND_LISTS
)
foreach(o kingdom arcade)
	string(TOUPPER ${o} O)
	add_custom_command(
		OUTPUT  ${CMAKE_BINARY_DIR}/${O}.OVL
		COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/ovl.py link ${CMAKE_BINARY_DIR}/${O}.OVL $<TARGET_FILE:bore2> ${CMAKE_C_COMPILER} ${TOOLCHAIN_NM} ${CMAKE_OBJCOPY} $<TARGET_OBJECTS:ovl_${o}>
		DEPENDS bore2 ovl_${o} $<TARGET_OBJECTS:ovl_${o}> ${CMAKE_SOURCE_DIR}/tools/ovl.py
		COMMAND_EXPAND_LISTS
	)
endforeach()
add_custom_target(overlays DEPENDS ${CMAKE_BINARY_DIR}/KINGDOM.OVL ${CMAKE_BINARY_DIR}/ARCADE.OVL)

# the voice clips (SPU ADPCM blob made by tools/build_audio.py)
# (the voice clips are VOICES.BIN on the disc: audio.c sends them to sound memory at start-up, so they take no main memory)

psn00bsdk_add_cd_image(
	iso
	bore2
	iso.xml
	DEPENDS bore2 system.cnf overlays ${CMAKE_BINARY_DIR}/KINGDOM.OVL ${CMAKE_BINARY_DIR}/ARCADE.OVL
)

# the disc as one file: BORE2.chd (DuckStation, SwanStation and the libretro PlayStation cores open it)
add_custom_command(
	OUTPUT  ${CMAKE_BINARY_DIR}/BORE2.chd
	COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/mkchd.py ${CMAKE_BINARY_DIR}/bore2.cue ${CMAKE_BINARY_DIR}/BORE2.chd
	DEPENDS iso ${CMAKE_BINARY_DIR}/bore2.bin ${CMAKE_SOURCE_DIR}/tools/mkchd.py
	COMMENT "Making BORE2.chd"
)
add_custom_target(chd DEPENDS ${CMAKE_BINARY_DIR}/BORE2.chd)

# a quick disc without the music, for testing in the emulator
psn00bsdk_add_cd_image(
	iso_lite
	bore2_lite
	iso_lite.xml
	DEPENDS bore2 system.cnf overlays ${CMAKE_BINARY_DIR}/KINGDOM.OVL ${CMAKE_BINARY_DIR}/ARCADE.OVL
)

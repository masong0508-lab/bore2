# BORE 2 (PlayStation) - built with PSn00bSDK
cmake_minimum_required(VERSION 3.21)

project(
	BORE2
	LANGUAGES    C CXX ASM
	VERSION      0.1.0
	DESCRIPTION  "BORE 2: the voxel life, in full 3D"
)

psn00bsdk_add_executable(bore2 GPREL
	src/main.c src/gfx.c src/atlas.c src/look.c src/voxel.c src/items.c src/world.c src/sim.c src/wants.c src/citizens.c src/audio.c src/font.c src/fmt.c src/pad.c src/crowd.c src/mem8.c src/bigmap.c src/pop.c src/far.c src/ui.c src/places.c src/home.c src/island.c src/nature.c)
target_include_directories(bore2 PRIVATE src assets)
if(NOT BORE_OPT)
	set(BORE_OPT -O2)
endif()
target_compile_options(bore2 PRIVATE -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-misleading-indentation -Wno-parentheses -Wno-strict-aliasing -fno-strict-aliasing)
get_target_property(BORE_SRCS bore2 SOURCES)
# the life's rules and the menus run a few times a frame at most: built small (-Os), which leaves more of the 2 MB for the world
set(BORE_SMALL src/main.c src/pop.c src/wants.c src/places.c src/look.c src/audio.c src/home.c src/island.c src/nature.c)
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

# the voice clips (SPU ADPCM blob made by tools/build_audio.py)
psn00bsdk_target_incbin(bore2 PRIVATE voices_bin assets/voices.bin)

psn00bsdk_add_cd_image(
	iso
	bore2
	iso.xml
	DEPENDS bore2 system.cnf
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
	DEPENDS bore2 system.cnf
)

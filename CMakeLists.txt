# BORE 2 (PlayStation) - built with PSn00bSDK
cmake_minimum_required(VERSION 3.21)

project(
	BORE2
	LANGUAGES    C CXX ASM
	VERSION      0.1.0
	DESCRIPTION  "BORE 2: the voxel life, in full 3D"
)

psn00bsdk_add_executable(bore2 GPREL
	src/main.c src/gfx.c src/atlas.c src/look.c src/voxel.c src/items.c src/world.c src/sim.c src/wants.c src/citizens.c src/audio.c src/font.c)
target_include_directories(bore2 PRIVATE src assets)
target_compile_options(bore2 PRIVATE -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-misleading-indentation -Wno-parentheses -O2)

# the voice clips (SPU ADPCM blob made by tools/build_audio.py)
psn00bsdk_target_incbin(bore2 PRIVATE voices_bin assets/voices.bin)

psn00bsdk_add_cd_image(
	iso
	bore2
	iso.xml
	DEPENDS bore2 system.cnf
)

# a quick disc without the music, for testing in the emulator
psn00bsdk_add_cd_image(
	iso_lite
	bore2_lite
	iso_lite.xml
	DEPENDS bore2 system.cnf
)

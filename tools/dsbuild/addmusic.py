#!/usr/bin/env python3
"""Appends assets/dsmusic.bin to a .nds, after everything else: padded to 512 bytes;
where the game's MUS_LOC marker (main.c) is patched with the start and size; the header's ROM size, capacity and CRC are fixed to match. usage: addmusic.py game.nds dsmusic.bin"""
import struct, sys
def crc16(d):
    c = 0xFFFF
    for b in d:
        c ^= b
        for _ in range(8): c = (c >> 1) ^ 0xA001 if c & 1 else c >> 1
    return c
rom = bytearray(open(sys.argv[1], 'rb').read()); mus = open(sys.argv[2], 'rb').read()
rom = rom[:struct.unpack_from('<I', rom, 0x80)[0]] if struct.unpack_from('<I', rom, 0x80)[0] <= len(rom) else rom
rom += bytes((-len(rom)) % 512); start = len(rom)
rom += mus; rom += bytes((-len(rom)) % 512)
key = b'BMDA' + bytes(8) + b'BDM.'
k = bytes(rom).find(key)
assert k >= 0, 'MUS_LOC not found in the ROM'
rom[k + 4:k + 12] = struct.pack('<II', start, len(mus))
size = len(rom); cap = 0
while (131072 << cap) < size: cap += 1
struct.pack_into('<I', rom, 0x80, size); rom[0x14] = cap
struct.pack_into('<H', rom, 0x15E, crc16(rom[:0x15E]))
open(sys.argv[1], 'wb').write(rom)
print('music at %d, rom %d bytes (capacity %d KB)' % (start, size, 128 << cap))

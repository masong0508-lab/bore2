#!/usr/bin/env python3
# vmuls.py vmu_save_A1.bin [outprefix]: list a VMU image's files (flycast's VMU file); with outprefix, each save's icon frames as PNGs
import struct, sys
d = open(sys.argv[1], 'rb').read()
blk = lambda n: d[n * 512:(n + 1) * 512]
root = blk(255)
fat_b, dir_b, dir_n = struct.unpack_from('<H', root, 0x46)[0], struct.unpack_from('<H', root, 0x4a)[0], struct.unpack_from('<H', root, 0x4c)[0]
fat = struct.unpack('<256H', blk(fat_b))
for b in range(dir_b, dir_b - dir_n, -1):
    db = blk(b)
    for e in range(16):
        ent = db[e * 32:(e + 1) * 32]
        if ent[0] == 0: continue
        first, name, size, hdr = struct.unpack_from('<H', ent, 2)[0], ent[4:16].split(b'\0')[0].decode('ascii', 'replace'), struct.unpack_from('<H', ent, 0x18)[0], struct.unpack_from('<H', ent, 0x1a)[0]
        data = b''; n = first
        while n < 256 and len(data) < size * 512: data += blk(n); n = fat[n]
        h = data[hdr * 512:]
        print('%-12s %3d blocks  "%s" "%s" app "%s" icons %d speed %d data %d bytes' % (name, size, h[0:16].decode('ascii', 'replace').strip(), h[16:48].decode('ascii', 'replace').strip(),
              h[48:64].split(b'\0')[0].decode('ascii', 'replace'), struct.unpack_from('<H', h, 64)[0], struct.unpack_from('<H', h, 66)[0], struct.unpack_from('<I', h, 72)[0]))
        if len(sys.argv) > 2:
            from PIL import Image
            pal = struct.unpack_from('<16H', h, 96); n_ic = struct.unpack_from('<H', h, 64)[0]
            rgba = [(((c >> 8) & 15) * 17, ((c >> 4) & 15) * 17, (c & 15) * 17, (c >> 12) * 17) for c in pal]
            im = Image.new('RGBA', (32 * n_ic + 8 * (n_ic - 1), 32), (40, 40, 60, 255))
            for k in range(n_ic):
                ic = h[128 + k * 512:128 + (k + 1) * 512]
                for y in range(32):
                    for x in range(32):
                        v = ic[y * 16 + x // 2]; v = (v >> 4) if x % 2 == 0 else (v & 15)
                        if rgba[v][3]: im.putpixel((k * 40 + x, y), rgba[v])
            im.resize((im.width * 4, im.height * 4), Image.NEAREST).save('%s_%s.png' % (sys.argv[2], name.strip().replace('.', '_')))

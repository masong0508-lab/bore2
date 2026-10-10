# ramsym.py <ram.bin> <sym>[:count] ... : print words of globals from a RAM dump (symbols from build/bore2.elf)
import struct, subprocess, sys
ram = open(sys.argv[1], 'rb').read()
out = subprocess.run(['/opt/mipsel/bin/mipsel-none-elf-nm', '/home/claude/bore2/build/bore2.elf'], capture_output=True, text=True).stdout
syms = {}
for l in out.split('\n'):
    p = l.split()
    if len(p) == 3: syms[p[2]] = int(p[0][-8:], 16)
for a in sys.argv[2:]:
    n, c = (a.split(':') + ['1'])[:2]; c = int(c)
    if n not in syms: print(n, '?'); continue
    o = syms[n] - 0x80000000
    print(n, list(struct.unpack('<%di' % c, ram[o:o + 4 * c])))

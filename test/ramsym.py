# ramsym.py <ram.bin> <sym>[+off][:count[:fmt]] ... : print globals from a RAM dump (symbols from $ELF, or build/bore2.elf)
# fmt: i int32 (the default), h/H 16 bits, b/B bytes, s a string of up to count bytes
import os, struct, subprocess, sys
ram = open(sys.argv[1], 'rb').read()
out = subprocess.run(['/opt/mipsel/bin/mipsel-none-elf-nm', os.environ.get('ELF', '/home/claude/bore2/build/bore2.elf')], capture_output=True, text=True).stdout
syms = {}
for l in out.split('\n'):
    p = l.split()
    if len(p) == 3: syms[p[2]] = int(p[0][-8:], 16)
for a in sys.argv[2:]:
    q = a.split(':'); n = q[0]; c = int(q[1]) if len(q) > 1 else 1; f = q[2] if len(q) > 2 else 'i'; off = 0
    if '+' in n: n, off = n.split('+'); off = int(off, 0)
    if n not in syms: print(n, '?'); continue
    o = syms[n] - 0x80000000 + off
    if f == 's': print(a, ram[o:o + c].split(b'\0')[0].decode('latin1')); continue
    print(a, list(struct.unpack('<%d%s' % (c, f), ram[o:o + struct.calcsize('<' + f) * c])))

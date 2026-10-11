# csym.py <ram.bin> <elf> sym[:count] ...: print words of globals
import struct,subprocess,sys
ram=open(sys.argv[1],'rb').read()
out=subprocess.run(['/opt/mipsel/bin/mipsel-none-elf-nm',sys.argv[2]],capture_output=True,text=True).stdout
sy={l.split()[2]:int(l.split()[0][-8:],16) for l in out.split('\n') if len(l.split())==3}
for a in sys.argv[3:]:
    n,c=(a.split(':')+['1'])[:2]; c=int(c); o=sy[n]-0x80000000
    print(n, list(struct.unpack('<%di'%c, ram[o:o+4*c])))

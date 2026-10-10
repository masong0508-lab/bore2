import struct,subprocess,sys
ram=open(sys.argv[1],'rb').read()
out=subprocess.run(['/opt/mipsel/bin/mipsel-none-elf-nm','/home/claude/bore2/build/bore2.elf'],capture_output=True,text=True).stdout
syms={l.split()[2]:int(l.split()[0][-8:],16) for l in out.split('\n') if len(l.split())==3}
p=struct.unpack('<I',ram[syms['vgrab_p']-0x80000000:][:4])[0]
from PIL import Image
im=Image.new('RGB',(624,32)); o=p-0x80000000
data=struct.unpack('<%dH'%(624*32),ram[o:o+624*32*2])
for y in range(32):
    for x in range(624):
        v=data[y*624+x]; im.putpixel((x,y),((v&31)<<3,((v>>5)&31)<<3,((v>>10)&31)<<3))
im.resize((1248,64),Image.NEAREST).save(sys.argv[2])

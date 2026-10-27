from PIL import Image, ImageDraw, ImageFont, ImageFilter
gold=Image.open('/mnt/user-data/attachments/2483a6af-7ed4-56bf-9a29-66f3f6dd76d4/dcaf7cf3-image.png').convert('RGBA')
# work in logo pixels: find the logo's pixel size (about 8 px per art pixel) -> make one art pixel = 1 unit
U=8; lw,lh=gold.width//U,gold.height//U
small=gold.resize((lw,lh),Image.NEAREST)
CW,CH=lw*2,lh*3
big=Image.new('RGBA',(CW,CH),(0,0,0,0)); bd=ImageDraw.Draw(big)
fnt=ImageFont.truetype('/usr/share/fonts/truetype/google-fonts/Poppins-Bold.ttf',int(lh*2.5))
bd.text((int(lw*0.5),-int(lh*0.2)),"2",font=fnt,fill=(255,255,255,255))
a=big.getchannel('A').point(lambda v:255 if v>110 else 0); bb=a.getbbox(); a=a.crop(bb); gw,gh=a.size
body=Image.new('RGBA',a.size,(0,0,0,0)); px=body.load(); ap=a.load()
for y in range(gh):
    t=y/max(1,gh-1); c=(int(255-60*t),int(92-70*t),int(139-40*t))
    for x in range(gw):
        if ap[x,y]:
            hl=(x<=1 and y>2) or y<1
            px[x,y]=(tuple(min(255,v+50) for v in c)+(255,)) if hl else c+(255,)
o2=Image.new('L',(gw+8,gh+8),0); o2.paste(a,(4,4)); o2=o2.filter(ImageFilter.MaxFilter(3)); sh=Image.new('L',(gw+8,gh+8),0); sh.paste(o2,(1,1))
two=Image.new('RGBA',(gw+8,gh+8),(0,0,0,0)); two.paste((22,10,30,255),(0,0),sh); two.paste((22,10,30,255),(0,0),o2); two.paste(body,(4,4),body)
tx=int(lw*0.66)
out=Image.new('RGBA',(max(tx+two.width,lw),max(lh,two.height)+4),(0,0,0,0))
ty=(out.height-two.height)//2
out.paste(two,(tx,ty),two)
out.paste(small,(0,(out.height-lh)//2),small)
bb=out.getbbox(); out=out.crop(bb)
out.resize((out.width*10,out.height*10),Image.NEAREST).save('/home/claude/bore2/docs/bore2_logo.png')
out.save('/home/claude/bore2/docs/bore2_logo_1x.png'); print(out.size)

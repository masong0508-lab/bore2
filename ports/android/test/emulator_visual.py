#!/usr/bin/env python3
"""Capture actual APK-rendered pixels and reject a blank or static 3D surface."""
import os
import re
import subprocess
import time
import xml.etree.ElementTree as ET
from pathlib import Path
from PIL import Image, ImageChops

OUT=Path(os.environ.get("BORE_VISUAL_DIR","android-render-evidence"))
OUT.mkdir(parents=True,exist_ok=True)
PACKAGE="com.bore2.game"

def adb(*args):
    return subprocess.run(["adb",*args],check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE).stdout

def bounds():
    adb("shell","uiautomator","dump","/sdcard/bore-ui.xml")
    xml=adb("exec-out","cat","/sdcard/bore-ui.xml")
    root=ET.fromstring(xml)
    ready=any("Drag to orbit" in n.get("text","") for n in root.iter("node"))
    node=next((n for n in root.iter("node") if n.get("content-desc","").startswith("San Boreas 3D neighborhood.")),None)
    if not ready or node is None:return None
    vals=[int(x) for x in re.findall(r"\d+",node.get("bounds",""))]
    if len(vals)!=4 or vals[2]-vals[0]<100 or vals[3]-vals[1]<100:return None
    return tuple(vals),xml

def wait_view(wide=None):
    until=time.monotonic()+70
    while time.monotonic()<until:
        try:
            r=bounds()
            if r:
                if wide is not None:
                    import io
                    with Image.open(io.BytesIO(adb("exec-out","screencap","-p"))) as pic:
                        if (pic.width>pic.height)!=wide:
                            time.sleep(1);continue
                return r
        except (subprocess.CalledProcessError,ET.ParseError,OSError):pass
        time.sleep(1)
    # Preserve the failing view instead of reporting an unexplained timeout.
    try:
        (OUT/"timeout.png").write_bytes(adb("exec-out","screencap","-p"))
        adb("shell","uiautomator","dump","/sdcard/bore-ui.xml")
        (OUT/"timeout.xml").write_bytes(adb("exec-out","cat","/sdcard/bore-ui.xml"))
        (OUT/"timeout-logcat.txt").write_bytes(adb("logcat","-d","-t","1000"))
    except (subprocess.CalledProcessError,OSError): pass
    raise AssertionError("3D scene did not become ready; see timeout evidence")

def capture(name,box,xml):
    raw=adb("exec-out","screencap","-p")
    file=OUT/(name+".png");file.write_bytes(raw)
    (OUT/(name+".xml")).write_bytes(xml)
    image=Image.open(file).convert("RGB")
    x,y,X,Y=box
    assert 0<=x<X<=image.width and 0<=y<Y<=image.height,(box,image.size)
    pic=image.crop(box)
    # Color histogram in a small spatially representative sample, not the
    # text or controls. Detect sky-only clear, black surface and uniform map.
    reduced=pic.resize((160,120))
    bins={}
    for r,g,b in reduced.getdata():
        key=(r//16,g//16,b//16)
        bins[key]=bins.get(key,0)+1
    colors=sum(n>=8 for n in bins.values())
    dominant=max(bins.values())/(160*120)
    assert colors>=12,(name,"too few material colors",colors)
    assert dominant<.90,(name,"surface mostly uniform",dominant)
    print(name,"viewport",pic.size,"material bins",colors,"dominant",round(dominant,3))
    return pic

def main():
    adb("shell","am","force-stop",PACKAGE)
    adb("shell","pm","clear",PACKAGE)
    adb("shell","settings","put","system","accelerometer_rotation","0")
    adb("shell","settings","put","system","user_rotation","0")
    adb("shell","am","start","-W","-n",PACKAGE+"/.MainActivity")
    box,xml=wait_view(False)
    time.sleep(2)
    a=capture("portrait",box,xml)
    x,y,X,Y=box
    adb("shell","input","swipe",str(x+(X-x)*3//4),str(y+(Y-y)//2),
        str(x+(X-x)//4),str(y+(Y-y)*3//5),"500")
    time.sleep(1)
    box2,xml2=wait_view(False)
    b=capture("portrait-orbit",box2,xml2)
    assert a.size==b.size
    diff=ImageChops.difference(a,b).resize((160,120))
    changed=sum(max(pixel)>12 for pixel in diff.getdata())/(160*120)
    assert changed>.04,("orbit did not visibly move the scene",changed)
    print("orbit changed pixel fraction",round(changed,3))
    adb("shell","settings","put","system","user_rotation","1")
    box,xml=wait_view(True)
    time.sleep(2)
    capture("landscape",box,xml)
    adb("shell","settings","put","system","user_rotation","0")
    box,xml=wait_view(False)
    time.sleep(1)
    capture("portrait-return",box,xml)
    print("Native Android renderer portrait/orbit/landscape/return regression passed")

if __name__=="__main__":main()

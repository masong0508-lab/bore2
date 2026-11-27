#!/usr/bin/env python3
"""Real APK UI regression: generated home, travel, lease, sleep, cook, reload."""
import re, subprocess, time
import xml.etree.ElementTree as ET
from emulator_visual import adb, wait_view, OUT, PACKAGE

def nodes():
    adb("shell","uiautomator","dump","/sdcard/bore-life-ui.xml")
    return list(ET.fromstring(adb("exec-out","cat","/sdcard/bore-life-ui.xml")).iter("node"))

def tap(predicate,label):
    deadline=time.monotonic()+25
    while time.monotonic()<deadline:
        try:
            for node in nodes():
                text=node.get("text","")
                if not predicate(text):continue
                b=[int(x) for x in re.findall(r"\d+",node.get("bounds",""))]
                if len(b)==4 and b[2]>b[0] and b[3]>b[1]:
                    adb("shell","input","tap",str((b[0]+b[2])//2),str((b[1]+b[3])//2))
                    return text
        except (subprocess.CalledProcessError,ET.ParseError):pass
        time.sleep(.4)
    raise AssertionError("Could not find tappable "+label)

def text_tap(text):return tap(lambda s:s==text,text)
def state():
    raw=adb("shell","run-as",PACKAGE,"cat","shared_prefs/world_preview.xml")
    return {n.get("name"):int(n.get("value")) for n in ET.fromstring(raw) if n.tag=="int"}
def wait_state(predicate):
    deadline=time.monotonic()+30
    while time.monotonic()<deadline:
        try:
            s=state()
            if predicate(s):return s
        except (subprocess.CalledProcessError,ET.ParseError):pass
        time.sleep(.3)
    raise AssertionError("Game save did not reach expected state")
def main():
    adb("shell","am","force-stop",PACKAGE)
    adb("shell","pm","clear",PACKAGE)
    adb("shell","settings","put","system","user_rotation","0")
    adb("shell","am","start","-W","-n",PACKAGE+"/.MainActivity")
    wait_view(False)
    text_tap("Find places")
    text_tap("Homes to rent")
    label=tap(lambda s:"blocks away" in s,"generated home listing")
    distance=int(re.search(r"(\d+) blocks away",label).group(1))
    if distance:
        text_tap("Travel here")
        wait_state(lambda s:s.get("trips",0)>=1)
        wait_view(False)
    text_tap("Home")
    text_tap("Move in")
    leased=wait_state(lambda s:s.get("homeX",-1)>=0)
    assert leased["homeX"]==leased["lifeX"] and leased["homeY"]==leased["lifeY"],leased
    assert leased["funds"]==175,leased
    text_tap("Home")
    text_tap("Sleep 8 hours")
    slept=wait_state(lambda s:s.get("homeNights",0)==1)
    assert slept["energy"]==100 and slept["minute"]==(leased["minute"]+480)%1440,slept
    assert slept["funds"]==leased["funds"]-12*((leased["minute"]+480)//1440),slept
    text_tap("Home")
    text_tap("Cook a meal ($5)")
    cooked=wait_state(lambda s:s.get("skill4",0)>=12)
    assert cooked["food"]>=slept["food"] and cooked["funds"]==slept["funds"]-5-12*((slept["minute"]+30)//1440),cooked
    adb("shell","input","keyevent","3")
    adb("shell","am","force-stop",PACKAGE)
    adb("shell","am","start","-W","-n",PACKAGE+"/.MainActivity")
    wait_view(False)
    loaded=state()
    for k in ("homeX","homeY","lifeX","lifeY","funds","food","energy","minute","homeNights","skill4"):
        assert loaded[k]==cooked[k],(k,loaded,cooked)
    (OUT/"life-home.png").write_bytes(adb("exec-out","screencap","-p"))
    print("Android real UI rent/travel/sleep/cooking/relaunch PASS",loaded)

if __name__=="__main__":
    try:main()
    except Exception:
        try:
            (OUT/"life-failure.png").write_bytes(adb("exec-out","screencap","-p"))
            (OUT/"life-failure-logcat.txt").write_bytes(adb("logcat","-d","-t","1000"))
        except Exception:pass
        raise

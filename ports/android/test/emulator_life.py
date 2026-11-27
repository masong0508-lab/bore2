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

def text_tap(text):return tap(lambda s:s.casefold()==text.casefold(),text)
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
    text_tap("Bed")
    slept=wait_state(lambda s:s.get("homeNights",0)==1)
    assert slept["energy"]==100 and slept["minute"]==(leased["minute"]+480)%1440,slept
    assert slept["funds"]==leased["funds"]-12*((leased["minute"]+480)//1440),slept
    text_tap("Kitchen")
    cooked=wait_state(lambda s:s.get("skill4",0)>=12)
    assert cooked["food"]>=slept["food"] and cooked["funds"]==slept["funds"]-5-12*((slept["minute"]+30)//1440),cooked
    adb("shell","input","keyevent","3")
    adb("shell","am","force-stop",PACKAGE)
    adb("shell","am","start","-W","-n",PACKAGE+"/.MainActivity")
    # Relaunch must restore the interior and its last committed walking cell.
    deadline=time.monotonic()+35
    while time.monotonic()<deadline:
        if any(n.get("text")=="Exit home" for n in nodes()):break
        time.sleep(.3)
    else:raise AssertionError("Home view not restored")
    loaded=state()
    for k in ("homeX","homeY","lifeX","lifeY","funds","food","energy","minute","homeNights","skill4","roomCell"):
        assert loaded[k]==cooked[k],(k,loaded,cooked)
    (OUT/"life-home.png").write_bytes(adb("exec-out","screencap","-p"))
    text_tap("Exit home")
    wait_state(lambda s:s.get("roomCell",0)==-1)
    wait_view(False)
    (OUT/"life-exterior-return.png").write_bytes(adb("exec-out","screencap","-p"))
    text_tap("Walk mode")
    started=wait_state(lambda s:s.get("tileX",-1)>=0 and s.get("tileY",-1)>=0)
    # The home arrival spawns on the street/yard strip at local (2,2),
    # one tile east is still outside the generated building footprint.
    text_tap("East")
    walked=wait_state(lambda s:s.get("tileX")==started["tileX"]+1 and s.get("tileY")==started["tileY"])
    assert walked["minute"]==(started["minute"]+1)%1440,walked
    assert walked["lifeX"]==walked["tileX"]//18 and walked["lifeY"]==walked["tileY"]//18,walked
    (OUT/"life-outdoor-walk.png").write_bytes(adb("exec-out","screencap","-p"))
    text_tap("Stop walking")
    adb("shell","input","keyevent","3")
    adb("shell","am","force-stop",PACKAGE)
    adb("shell","am","start","-W","-n",PACKAGE+"/.MainActivity")
    wait_view(False)
    restored=state()
    for k in ("tileX","tileY","lifeX","lifeY","minute","funds"):
        assert restored[k]==walked[k],(k,restored,walked)
    print("Android real UI outdoor step and relaunch PASS",restored)
    text_tap("Find places")
    text_tap("Food and groceries")
    label=tap(lambda s:"blocks away" in s,"food venue listing")
    distance=int(re.search(r"(\d+) blocks away",label).group(1))
    if distance:
        text_tap("Travel here")
        wait_state(lambda s:s.get("trips",0)>restored["trips"])
        wait_view(False)
    text_tap("Enter")
    entered=wait_state(lambda s:s.get("insideLot",-1) in (6,7,10,15,21,25,30,31) and s.get("roomCell",-1)>=0)
    assert entered["funds"]==restored["funds"],entered
    text_tap("Order meal")
    served=wait_state(lambda s:s.get("funds",0)==entered["funds"]-8)
    assert served["minute"]==(entered["minute"]+30)%1440 and served["food"]>=entered["food"],served
    (OUT/"life-venue-meal.png").write_bytes(adb("exec-out","screencap","-p"))
    adb("shell","input","keyevent","3")
    adb("shell","am","force-stop",PACKAGE)
    adb("shell","am","start","-W","-n",PACKAGE+"/.MainActivity")
    deadline=time.monotonic()+35
    while time.monotonic()<deadline:
        if any(n.get("text")=="Exit building" for n in nodes()):break
        time.sleep(.3)
    else:raise AssertionError("Commercial interior not restored")
    reloaded=state()
    for k in ("insideLot","insideX","insideY","roomCell","funds","minute"):
        assert reloaded[k]==served[k],(k,reloaded,served)
    text_tap("Exit building")
    wait_state(lambda s:s.get("roomCell",0)==-1)
    wait_view(False)
    print("Android real UI venue counter service and reload PASS",served)
    text_tap("Find places")
    text_tap("Tall buildings")
    label=tap(lambda s:"floors" in s and "blocks away" in s,"multi-storey public building")
    distance=int(re.search(r"(\d+) blocks away",label).group(1))
    before=state()
    if distance:
        text_tap("Travel here")
        wait_state(lambda s:s.get("trips",0)>before["trips"])
        wait_view(False)
    text_tap("Enter")
    ground=wait_state(lambda s:s.get("insideLot",-1) in (14,15) and s.get("roomFloor",-1)==0)
    text_tap("Upstairs")
    upstairs=wait_state(lambda s:s.get("roomFloor")==1)
    assert upstairs["minute"]==(ground["minute"]+2)%1440,upstairs
    assert upstairs["lifeX"]==ground["lifeX"] and upstairs["lifeY"]==ground["lifeY"],upstairs
    (OUT/"life-upstairs.png").write_bytes(adb("exec-out","screencap","-p"))
    adb("shell","input","keyevent","3")
    adb("shell","am","force-stop",PACKAGE)
    adb("shell","am","start","-W","-n",PACKAGE+"/.MainActivity")
    deadline=time.monotonic()+35
    while time.monotonic()<deadline:
        if any(n.get("text")=="Downstairs" for n in nodes()):break
        time.sleep(.3)
    else:raise AssertionError("Upper floor not restored")
    loaded_floor=state()
    for k in ("insideX","insideY","insideLot","roomFloor","roomCell","minute"):
        assert loaded_floor[k]==upstairs[k],(k,loaded_floor,upstairs)
    text_tap("Downstairs")
    down=wait_state(lambda s:s.get("roomFloor")==0)
    assert down["roomCell"]==upstairs["roomCell"],down
    text_tap("Exit building")
    wait_state(lambda s:s.get("roomFloor",0)==-1 and s.get("roomCell",0)==-1)
    wait_view(False)
    print("Android real UI aligned stair transition and upper floor reload PASS",upstairs)
    print("Android real UI rent/travel/sleep/cooking/relaunch PASS",loaded)

if __name__=="__main__":
    try:main()
    except Exception:
        try:
            (OUT/"life-failure.png").write_bytes(adb("exec-out","screencap","-p"))
            (OUT/"life-failure-logcat.txt").write_bytes(adb("logcat","-d","-t","1000"))
        except Exception:pass
        raise

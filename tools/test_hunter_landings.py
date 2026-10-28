"""Replay Bit/Byte from prepared approaches in a disposable debug profile.

The caller starts a muted test host. No enemy coordinates, collision state,
camera limits, spawn flags or encounter results are written by this check.
Prerequisite assists are recorded in the supplied approach snapshots.
"""
import argparse
import json
from pathlib import Path
import shutil
import sys
from hunter_approach import validate_door_graphics


def run_case(driver, profile, state, name, actor_id):
    profile, state = Path(profile), Path(state)
    (profile/'saves').mkdir(exist_ok=True)
    shutil.copy2(state, profile/'saves/save0.sav')
    driver.command('clear_controller')
    driver.command('loadstate 0')
    for _ in range(3): driver.command('step 1')
    def hunters():
        r=driver.read(0xd18,0x800)
        return [dict(dp=0xd18+i, state=r[i:i+4].hex(), id=r[i+10],
                     y=int.from_bytes(r[i+8:i+10],'little'), hp=r[i+0x27])
                for i in range(0,0x800,64) if r[i] and r[i+10] in (0x46,0x47)]
    assert not hunters(), 'Approach already contains a hunter'
    assert driver.byte(0x1fd8)&15 == 0, 'Approach has completed encounter flags'
    start=dict(x=driver.word(0x9dd),y=driver.word(0x9e0),
               camera=[driver.word(0x1e5d),driver.word(0x1e60)])
    assert 0x1400 <= start['x'] < 0x1500, start
    graphics = validate_door_graphics(driver)
    samples=[]
    for n in range(0,660,10):
        driver.byte(0x9ff,32)
        driver.step(10,'right')
        samples.append(dict(frames=n+10,hunters=hunters()))
    live=hunters()
    assert len(live)==1 and live[0]['id']==actor_id, live
    actor=live[0]
    # Measured retail sprite feet relative to each object's world origin.
    foot_offset=24 if actor_id==0x46 else 40
    floor=driver.word(0x9e0)+17
    foot=actor['y']+foot_offset
    assert abs(foot-floor)<=1, (name,actor,foot,floor)
    assert driver.byte(0x1fd8)&15 == 0
    driver.snapshot(name+'-landing')
    for _ in range(50): driver.step(1,'b'); driver.step(10)
    live=hunters()
    assert len(live)==1 and live[0]['state'][2:4]=='02' and live[0]['hp']>0, live
    driver.snapshot(name+'-active')
    result=dict(name=name, start=start, graphics=graphics, landing=actor, feet=foot, floor=floor,
                active=live, outcome_flags=driver.byte(0x1fd8),samples=samples,
                passed=True)
    (profile/(name+'-landing-result.json')).write_text(json.dumps(result,indent=2))
    return result


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--port',required=True,type=int)
    ap.add_argument('--profile',required=True,type=Path)
    ap.add_argument('--states',required=True,type=Path)
    ap.add_argument('--framework',type=Path,default=Path(__file__).resolve().parents[1]/'snesrecomp')
    args=ap.parse_args()
    sys.path.insert(0,str(args.framework/'tools'))
    from sneslib.client import DebugClient
    from sneslib.route import FrameDriver
    client=DebugClient(args.port,timeout=20)
    driver=FrameDriver(client,args.profile)
    try:
        result=[run_case(driver,args.profile,args.states/(name+'.sav'),name,actor)
                for name,actor in [('x3-byte',0x47),('x3-bit',0x46)]]
        print(json.dumps(result,indent=2))
    finally: driver.close()

if __name__=='__main__': main()

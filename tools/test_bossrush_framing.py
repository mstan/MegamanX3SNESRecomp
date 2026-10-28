"""Check isolated rematch framing and restored adaptive hub presentation.

Supply current-build live-room and hub snapshots to an already running muted
debug host with widescreen enabled. No game RAM or encounter results are edited.
Native teleports, defeats and cumulative flags are separate gameplay checks.
"""
import argparse
import json
from pathlib import Path
import shutil
import struct
import sys


def run_case(driver, profile, state, expected_width, label):
    profile=Path(profile)
    (profile/'saves').mkdir(exist_ok=True)
    shutil.copy2(state,profile/'saves/save0.sav')
    driver.command('clear_controller');driver.command('loadstate 0')
    for _ in range(3):driver.command('step 1')
    driver.command('screenshot '+label+'.bmp')
    width,height=struct.unpack_from('<ii',(profile/(label+'.bmp')).read_bytes(),18)
    result=dict(stage=driver.byte(0x1fae),checkpoint=driver.byte(0x1fb5),
                size=[width,abs(height)],expected_width=expected_width,
                rematch_flags=driver.byte(0x1fda))
    result['passed']=result['stage']==12 and result['size']==[expected_width,224]
    (profile/(label+'.json')).write_bytes((json.dumps(result,indent=2)+'\n').encode())
    assert result['passed'],result
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',required=True,type=int)
    parser.add_argument('--profile',required=True,type=Path)
    parser.add_argument('--room-state',required=True,type=Path)
    parser.add_argument('--hub-state',required=True,type=Path)
    parser.add_argument('--corridor-state',required=True,type=Path)
    parser.add_argument('--wide-width',required=True,type=int)
    parser.add_argument('--framework',type=Path,
                        default=Path(__file__).resolve().parents[1]/'snesrecomp')
    args=parser.parse_args();sys.path.insert(0,str(args.framework/'tools'))
    from sneslib.client import DebugClient
    from sneslib.route import FrameDriver
    driver=FrameDriver(DebugClient(args.port,timeout=20),args.profile)
    try:
        print(json.dumps([
            run_case(driver,args.profile,args.room_state,256,'rematch-native-frame'),
            run_case(driver,args.profile,args.hub_state,args.wide_width,'hub-wide-frame'),
            run_case(driver,args.profile,args.corridor_state,args.wide_width,'corridor-wide-frame')]))
    finally:driver.close()


if __name__=='__main__':main()

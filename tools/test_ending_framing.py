"""Verify fixed epilogue framing and restored widescreen cast on a debug host.

Use disposable current-build snapshots and a muted private profile with the
widescreen feature enabled. This checks presented dimensions and current modes;
the real final clear and ending progression require separate gameplay evidence.
"""
import argparse
import json
from pathlib import Path
import shutil
import struct
import sys


def check(driver, profile, state, phase, width, label):
    shutil.copy2(state, profile/'saves/save0.sav')
    driver.command('clear_controller');driver.command('loadstate 0')
    for _ in range(3):driver.command('step 1')
    driver.command('screenshot '+label+'.bmp')
    actual_width,height=struct.unpack_from('<ii',(profile/(label+'.bmp')).read_bytes(),18)
    mode=[driver.byte(0xd1),driver.byte(0xd2)]
    result=dict(case=label,mode=mode,size=[actual_width,abs(height)],expected_width=width)
    result['passed']=mode==[6,phase] and result['size']==[width,224]
    assert result['passed'],result
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',required=True,type=int)
    parser.add_argument('--profile',required=True,type=Path)
    parser.add_argument('--epilogue-state',required=True,type=Path)
    parser.add_argument('--cast-state',required=True,type=Path)
    parser.add_argument('--wide-width',required=True,type=int)
    parser.add_argument('--framework',type=Path,
                        default=Path(__file__).resolve().parents[1]/'snesrecomp')
    args=parser.parse_args();sys.path.insert(0,str(args.framework/'tools'))
    from sneslib.client import DebugClient
    from sneslib.route import FrameDriver
    (args.profile/'saves').mkdir(exist_ok=True)
    driver=FrameDriver(DebugClient(args.port,timeout=20),args.profile)
    try:
        print(json.dumps([
            check(driver,args.profile,args.epilogue_state,0,256,'epilogue-native-frame'),
            check(driver,args.profile,args.cast_state,2,args.wide_width,'cast-wide-frame')]))
    finally:driver.close()


if __name__=='__main__':main()

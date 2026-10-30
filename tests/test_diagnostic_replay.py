#!/usr/bin/env python3
"""ROM-backed default diagnostics acceptance; all artifacts stay in --work-dir."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,required=True)
    p.add_argument('--state-exe',type=Path,required=True)
    p.add_argument('--rom',type=Path,required=True)
    p.add_argument('--work-dir',type=Path,required=True)
    a=p.parse_args()
    exe,state_exe,rom=[v.resolve(strict=True) for v in (a.exe,a.state_exe,a.rom)]
    work=a.work_dir.resolve();work.mkdir(parents=True,exist_ok=False)
    helper=Path(__file__).resolve().parents[1]/'tools/replay_diagnostic.py'
    evidence=[]
    base_env={**os.environ,'SDL_VIDEODRIVER':'dummy','SDL_AUDIODRIVER':'dummy',
              'SNESRECOMP_RUNAHEAD':'0','SNESRECOMP_REWIND_INTERVAL':'1','MMX_REPLAY_FAST':'1'}
    base_env.pop('MMX_REPLAY_RECORD',None)  # Verify the actual default.

    def run(label,argv,cwd,env=base_env,good=True,contains=None):
        start=time.monotonic()
        result=subprocess.run([str(x) for x in argv],cwd=cwd,env=env,
                              stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=180)
        (work/(label+'.log')).write_bytes(result.stdout)
        text=result.stdout.decode(errors='replace')
        if (result.returncode==0)!=good or (contains and contains not in text):
            raise AssertionError(label+f' exit={result.returncode}\n'+text[-8000:])
        evidence.append(dict(check=label,exit_code=result.returncode,seconds=time.monotonic()-start))
        print('PASS',label,flush=True)
        return text

    def playback(label,capture,good=True,contains=None):
        return run(label,[sys.executable,helper,capture,'--exe',exe,'--rom',rom,
                          '--headless','--work-dir',work/label],work,good=good,
                   contains=contains or ('all recorded checkpoints matched' if good else None))

    state=work/'state';state.mkdir()
    env={**base_env,'MMX_REPLAY_DIR':str(state)}
    run('current-build-save-rewind',[state_exe,rom],state,env,contains='MMX STATE CHECKS PASSED')
    run('fresh-process-save',[state_exe,rom,'load'],state,env,contains='MMX STATE CHECKS PASSED')
    captures=sorted(state.glob('*.replay'))
    assert len(captures)>=3,'default recording and segment rotation must work'
    # Existing state checks compare complete serialized machines with recording
    # active, then restore files in another process and commit the rewind ring.
    for i,capture in enumerate(captures):
        playback('state-replay-'+str(i),capture)
    all_lines='\n'.join(c.read_text() for c in captures)
    assert all_lines.count('\nS ')>len(captures),'save/rewind discontinuities carry checkpoints'

    # Exercise the real desktop with configuration and an effective widescreen
    # selection, reset, inputs and renderer changes; never write owner settings.
    desktop=work/'desktop';desktop.mkdir()
    (desktop/'config.ini').write_text('[General]\nDisableFrameDelay = 1\nFullscreen = 0\n[Sound]\nAudioEnable = 1\n')
    catalog=desktop/'mods/preloaded'
    shutil.copytree(exe.parent/'mods/preloaded/packages',catalog/'packages')
    manifest=next((catalog/'packages').glob('*/*/manifest.toml'))
    import tomllib
    package=tomllib.loads(manifest.read_text())
    feature=package['feature'][0]
    (catalog/'state.toml').write_text('format_version = 1\n[[feature]]\npackage_id = '+json.dumps(package['id'])+'\nid = '+json.dumps(feature['id'])+'\nenabled = true\n')
    script=desktop/'inputs.txt'
    script.write_text('wait 200\npress start\nwait 150\npress start\nwait 650\nrenderer toggle\nwait 40\nreset\nwait 120\nquit\n')
    run('desktop-default-on',[exe,'--rom',rom,'--config',desktop/'config.ini','--no-launcher','--script',script],
        desktop,{**base_env,'MMX_REPLAY_DIR':str(desktop)})
    desktop_captures=sorted(desktop.glob('*.replay'))
    assert len(desktop_captures)>=2
    assert any('widescreen' in c.with_name(c.name+'.mods.txt').read_text() for c in desktop_captures)
    for i,capture in enumerate(desktop_captures):playback('desktop-replay-'+str(i),capture)
    capture=desktop_captures[0]
    original=capture.read_text().splitlines()
    damaged=work/'damaged';damaged.mkdir()
    def variant(name,lines,corrupt=False):
        target=damaged/(name+'.replay');target.write_text('\n'.join(lines)+'\n')
        for suffix in ('.sav','.config.ini','.mods.toml','.mods.txt'):
            source=capture.with_name(capture.name+suffix)
            if source.exists():shutil.copy2(source,str(target)+suffix)
        if corrupt:
            data=bytearray(Path(str(target)+'.sav').read_bytes());data[len(data)//2]^=1
            Path(str(target)+'.sav').write_bytes(data)
        return target
    lines=original.copy();i=next(i for i,l in enumerate(lines) if l.startswith('H '))
    fields=lines[i].split();fields[2]='00000000';lines[i]=' '.join(fields)
    playback('reject-divergence',variant('divergence',lines),False,'FIRST DIVERGENCE')
    assert (work/'reject-divergence/replay-divergence.sav').exists()
    playback('reject-checkpoint',variant('checkpoint',original,True),False,'damaged or incompatible')
    lines=original.copy();lines[2]='ROM 00000000'
    playback('reject-ROM',variant('rom',lines),False,'different ROM')
    lines=original.copy();lines[0]='MMX3REPLAY 1' if lines[0]=='MMX2REPLAY 1' else 'MMX2REPLAY 1'
    playback('reject-title',variant('title',lines),False,'invalid header')
    playback('reject-incomplete',variant('incomplete',original[:i]),False,'incomplete or invalid checkpoint')

    for name,override in [('opt-out',{'MMX_REPLAY_RECORD':'0'}),('unwritable',{'MMX_REPLAY_DIR':str(desktop/'config.ini')})]:
        folder=work/name;folder.mkdir();(folder/'config.ini').write_text('[General]\nFullscreen = 0\n')
        run(name,[exe,'--rom',rom,'--config',folder/'config.ini','--no-launcher'],folder,
            {**base_env,'SNESRECOMP_RUN_FRAMES':'30','MMX_REPLAY_DIR':str(folder),**override})
        assert not list(folder.glob('*.replay'))

    bounded=work/'bounded';bounded.mkdir()
    run('bounded-history',[state_exe,rom],bounded,
        {**base_env,'MMX_REPLAY_DIR':str(bounded),'MMX_REPLAY_HISTORY_MIB':'2'},contains='MMX STATE CHECKS PASSED')
    retained=sorted(bounded.glob('*.replay'))
    assert retained and sum(f.stat().st_size for f in bounded.glob('*.replay.sav'))<=2*1024*1024
    for i,capture in enumerate(retained):playback('bounded-replay-'+str(i),capture)
    (work/'results.json').write_text(json.dumps(dict(passed=True,checks=evidence),indent=2)+'\n')
    print('DIAGNOSTIC ACCEPTANCE PASSED',flush=True)

if __name__=='__main__':main()

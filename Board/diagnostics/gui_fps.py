"""Measure the matching running target ELF via hotplug SWD, without halt/reset.

--swipes or --routes opts into navigation tests. They change no power settings.
Without either option, all target accesses are reads. Rendering cadence excludes
the explicitly reported bounded snapshot-preparation phase and idle frames.
"""
import argparse
import ctypes as C
import json
import os
from pathlib import Path
import struct
import subprocess
import time

from phy_probe import Parameters

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--seconds', type=float, default=10)
parser.add_argument('--swipes', type=int, default=0)
parser.add_argument('--routes', type=int, default=0,help='Opt into menu, battery, PD, output monitor, charger, appearance, network and diagnostics navigation')
parser.add_argument('--output', type=Path)
args = parser.parse_args()
if not 1 <= args.seconds <= 60 or not 0 <= args.swipes <= 100 or not 0 <= args.routes <= 100 or (args.routes and args.swipes):
    parser.error('Use 1..60 seconds and either 0..100 swipes or 0..100 routes')
root = Path(__file__).resolve().parents[2]
binpath = Path('C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin')
nm = 'C:/TouchGFX/4.26.1/env/MinGW/msys/1.0/gnu-arm-gcc/bin/arm-none-eabi-nm.exe'
output = subprocess.check_output([nm, '-n', str(root / 'Appli/TouchGFX/build/bin/target.elf')], text=True)
symbols = {p[2]: int(p[0], 16) for line in output.splitlines()
           if len(p := line.split()) == 3 and all(c in '0123456789abcdefABCDEF' for c in p[0])}
handle = os.add_dll_directory(str(binpath))
api = C.CDLL(str(binpath / 'CubeProgrammer_API.dll'))
api.getStLinkList.argtypes = [C.POINTER(C.POINTER(Parameters)), C.c_int]
api.connectStLink.argtypes = [Parameters]
api.readMemory.argtypes = [C.c_uint, C.POINTER(C.POINTER(C.c_ubyte)), C.c_uint]
api.writeMemory.argtypes = [C.c_uint, C.c_char_p, C.c_uint]
api.freeLibraryMemory.argtypes = [C.c_void_p]
api.setLoadersPath.argtypes = [C.c_char_p]
api.setLoadersPath(str(binpath).encode())
api.setVerbosityLevel(0)


def read(address, size):
    data = C.POINTER(C.c_ubyte)()
    if api.readMemory(address, C.byref(data), size):
        raise RuntimeError(f'SWD read failed at {address:08x}')
    try:
        return C.string_at(data, size)
    finally:
        api.freeLibraryMemory(data)


def words(name, count):
    return struct.unpack('<' + 'I' * count, read(symbols[name], count * 4))


probes = C.POINTER(Parameters)()
if api.getStLinkList(C.byref(probes), 0) != 1:
    raise RuntimeError('Expected one ST-Link')
probe = probes[0]
probe.ap, probe.mode, probe.freq = 1, 1, 4000
if api.connectStLink(probe):
    raise RuntimeError('Hotplug SWD connection failed')
try:
    if args.swipes or args.routes:
        name='BoardGraphicsRouteBenchmark' if args.routes else 'BoardGraphicsSwipeBenchmark'
        for flag in ['BoardGraphicsRouteBenchmark','BoardGraphicsSwipeBenchmark']:
            if flag in symbols and words(flag,1)[0]:raise RuntimeError('A navigation benchmark is already running')
        if api.writeMemory(symbols[name], struct.pack('<I', args.routes or args.swipes), 4):
            raise RuntimeError('Cannot start navigation benchmark')
    previous = words('BoardGraphicsProfile', 7)[0]
    preparation_start=words('BoardGraphicsPreparationProfile',4) if 'BoardGraphicsPreparationProfile' in symbols else (0,0,0,0)
    started = time.monotonic()
    frames = []
    windows = []
    window_start, window_count = started, previous
    while time.monotonic() - started < args.seconds:
        time.sleep(.1)
        current = words('BoardGraphicsProfile', 7)[0]
        # Read after the count: the most recently committed slot is complete.
        ring = words('BoardGraphicsFrameTimes', 384)
        count = (current - previous) & 0xffffffff
        if count > 128:
            raise RuntimeError('Sample ring overrun or target reset')
        for index in range(previous, previous + count):
            render, period, motion = ring[index % 128 * 3:index % 128 * 3 + 3]
            frames.append(dict(frame=index, render_us=render, period_us=period, motion=bool(motion)))
        previous = current
        now = time.monotonic()
        if now - window_start >= 1:
            windows.append(dict(seconds=round(now-started, 3), fps=round(((current-window_count)&0xffffffff)/(now-window_start), 2)))
            window_start, window_count = now, current
    moving = [f for f in frames if f['motion']]
    def summarize(samples):
        if not samples:
            return None
        periods = sorted(f['period_us'] for f in samples)
        renders = sorted(f['render_us'] for f in samples)
        return dict(frames=len(samples), fps_by_begin_period=round(1e6*len(periods)/sum(periods), 2),
                    period_us_median=periods[len(periods)//2], period_us_max=max(periods),
                    render_us_mean=round(sum(renders)/len(renders)),
                    render_us_p95=renders[min(len(renders)-1, int(len(renders)*.95))],
                    render_us_max=max(renders), periods_over_20ms=sum(p > 20000 for p in periods))
    result = dict(elapsed_s=round(time.monotonic()-started, 3), windows=windows,
                  all=summarize(frames), motion=summarize(moving),
                  swipes_remaining=words('BoardGraphicsSwipeBenchmark', 1)[0],
                  faults=read(0xE000ED28, 8).hex())
    if 'BoardGraphicsPreparationProfile' in symbols:
        preparation=words('BoardGraphicsPreparationProfile',4)
        result['preparation']=dict(ticks=(preparation[0]-preparation_start[0])&0xffffffff,
                                   work_us=(preparation[1]-preparation_start[1])&0xffffffff,
                                   lifetime_max_tick_us=preparation[2],last_tick_us=preparation[3])
    if 'BoardGraphicsRouteBenchmark' in symbols:result['routes_remaining']=words('BoardGraphicsRouteBenchmark',1)[0]
    print(json.dumps(result, indent=2))
    if args.output:
        args.output.write_text(json.dumps(dict(summary=result, frames=frames), indent=2), encoding='utf-8')
finally:
    api.disconnect()
    handle.close()

"""Read live network diagnostics using matching target ELF symbols; no writes/reset."""
import ctypes as C
import os
from pathlib import Path
import subprocess
import struct
from phy_probe import Parameters
root=Path(__file__).resolve().parents[2]
binpath=Path('C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin')
nm=Path('C:/TouchGFX/4.26.1/env/MinGW/msys/1.0/gnu-arm-gcc/bin/arm-none-eabi-nm.exe')
output=subprocess.check_output([str(nm),'-n',str(root/'Appli/TouchGFX/build/bin/target.elf')],text=True)
symbols={parts[2]:int(parts[0],16) for line in output.splitlines() if len(parts:=line.split())==3 and all(c in '0123456789abcdefABCDEF' for c in parts[0])}
handle=os.add_dll_directory(str(binpath));api=C.CDLL(str(binpath/'CubeProgrammer_API.dll'))
api.getStLinkList.argtypes=[C.POINTER(C.POINTER(Parameters)),C.c_int]
api.connectStLink.argtypes=[Parameters]
api.readMemory.argtypes=[C.c_uint,C.POINTER(C.POINTER(C.c_ubyte)),C.c_uint]
api.freeLibraryMemory.argtypes=[C.c_void_p]
api.setLoadersPath.argtypes=[C.c_char_p];api.setLoadersPath(str(binpath).encode());api.setVerbosityLevel(0)
def read(address,n):
 data=C.POINTER(C.c_ubyte)();rc=api.readMemory(address,C.byref(data),n)
 if rc:raise RuntimeError(f'read {address:x}: {rc}')
 try:return C.string_at(data,n)
 finally:api.freeLibraryMemory(data)
probes=C.POINTER(Parameters)();count=api.getStLinkList(C.byref(probes),0)
if count!=1:raise RuntimeError(f'Expected one probe, got {count}')
p=probes[0];p.ap=1;p.mode=1;p.freq=1000
if api.connectStLink(p):raise RuntimeError('SWD connect failed')
try:
 for name,n in [('BoardNetDiagnostics',13),('BoardGraphicsProfile',7),('BoardBacklightReady',1),('uwTick',1)]:
  values=struct.unpack('<'+'I'*n,read(symbols[name],n*4));print(name,values)
  if name=='BoardNetDiagnostics':print('IPv4','.'.join(map(str,struct.pack('<I',values[2]))))
 print('CFSR/HFSR',read(0xE000ED28,8).hex())
finally:api.disconnect();handle.close()

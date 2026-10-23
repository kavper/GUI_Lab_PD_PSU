"""LAN8742A bring-up diagnostic via CubeProgrammer 2.23 API, Windows x64.

Run only while the board's Ethernet driver is disabled. Temporarily enables
ETH/GPIO clocks and muxes PA2/PC1 to MDIO/MDC; restores the changed bits.
No Flash writes, core halt/reset or PSU output/setpoint commands are issued.
--test tdr/restart additionally changes PHY registers, with restoration for TDR.
"""
import argparse
import ctypes as C
import os
from pathlib import Path
import struct
import time


class Frequencies(C.Structure):
    _fields_ = [('jtag', C.c_uint * 12), ('njtag', C.c_uint),
                ('swd', C.c_uint * 12), ('nswd', C.c_uint)]


class Parameters(C.Structure):
    _fields_ = [('port', C.c_int), ('index', C.c_int), ('serial', C.c_char * 33),
                ('firmware', C.c_char * 20), ('voltage', C.c_char * 5),
                ('apcount', C.c_int), ('ap', C.c_int), ('mode', C.c_int),
                ('reset', C.c_int), ('old', C.c_int), ('freqs', Frequencies),
                ('freq', C.c_int), ('bridge', C.c_int), ('shared', C.c_int),
                ('board', C.c_char * 100), ('sleep', C.c_int), ('speed', C.c_int)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--test', choices=['status', 'restart', 'tdr'], default='status')
    parser.add_argument('--serial')
    parser.add_argument('--cube', type=Path, default=Path(
        'C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer'))
    args = parser.parse_args()
    directory = (args.cube / 'bin').resolve()
    dll_directory = os.add_dll_directory(str(directory))
    api = C.CDLL(str(directory / 'CubeProgrammer_API.dll'))
    api.getStLinkList.argtypes = [C.POINTER(C.POINTER(Parameters)), C.c_int]
    api.connectStLink.argtypes = [Parameters]
    api.readMemory.argtypes = [C.c_uint, C.POINTER(C.POINTER(C.c_ubyte)), C.c_uint]
    api.writeMemory.argtypes = [C.c_uint, C.c_char_p, C.c_uint]
    api.freeLibraryMemory.argtypes = [C.c_void_p]
    api.setLoadersPath.argtypes = [C.c_char_p]
    api.setLoadersPath(str(directory).encode())
    api.setVerbosityLevel(0)

    def read(address):
        data = C.POINTER(C.c_ubyte)()
        result = api.readMemory(address, C.byref(data), 4)
        if result:
            raise RuntimeError(f'read 0x{address:08X}: {result}')
        try:
            return struct.unpack('<I', C.string_at(data, 4))[0]
        finally:
            api.freeLibraryMemory(data)

    def write(address, value):
        result = api.writeMemory(address, struct.pack('<I', value), 4)
        if result:
            raise RuntimeError(f'write 0x{address:08X}: {result}')

    saved = {}

    def change(address, mask, value):
        old = read(address)
        if address not in saved:
            saved[address] = (mask, old)
        else:
            first_mask, first_old = saved[address]
            saved[address] = (first_mask | mask, first_old)
        write(address, (old & ~mask) | (value & mask))

    def alternate(base, pin):
        change(base, 3 << (pin * 2), 2 << (pin * 2))
        change(base + 4, 1 << pin, 0)
        change(base + 8, 3 << (pin * 2), 3 << (pin * 2))
        change(base + 12, 3 << (pin * 2), 1 << (pin * 2))
        shift = (pin % 8) * 4
        change(base + 0x20 + (pin // 8) * 4, 15 << shift, 11 << shift)

    def wait_mdio():
        deadline = time.monotonic() + .25
        while read(0x40028200) & 1:
            if time.monotonic() > deadline:
                raise RuntimeError('MDIO busy timeout')

    def mdio(address, register):
        # HCLK=300 MHz, CSR/124 => MDC below 2.5 MHz; Clause 22 read.
        write(0x40028200, (address << 21) | (register << 16) | 0x50D)
        wait_mdio()
        return read(0x40028204) & 0xFFFF

    def phy_write(address, register, value):
        write(0x40028204, value)
        write(0x40028200, (address << 21) | (register << 16) | 0x505)
        wait_mdio()

    probes = C.POINTER(Parameters)()
    count = api.getStLinkList(C.byref(probes), 0)
    indices = [i for i in range(max(count, 0))
               if not args.serial or probes[i].serial.decode() == args.serial]
    if len(indices) != 1:
        raise RuntimeError('Select exactly one probe with --serial')
    probe = probes[indices[0]]
    probe.port, probe.ap, probe.mode = 1, 1, 1  # SWD, AP1, hot plug
    probe.freq, probe.shared = 1000, 0
    if api.connectStLink(probe):
        raise RuntimeError('Cannot connect to H7RS AP1')
    try:
        print('PROBE', probe.serial.decode(), flush=True)
        change(0x58024538, 0x38000, 0x38000)
        if read(0x40028110) != 0x3242:
            raise RuntimeError('Unexpected MAC version; expected this H7RS board')
        if read(0x40028000) & 3:
            raise RuntimeError('MAC is running; stop its driver before probing')
        change(0x58024540, 0x1D, 0x1D)
        change(0x58021000, 3 << 8, 0)
        print('PHY_RESET_PE4', bool(read(0x58021010) & 16), flush=True)
        change(0x58020C00, 3 << 14, 0)
        change(0x58020C0C, 3 << 14, 0)
        bits = [bool(read(0x58020C10) & 128) for _ in range(128)]
        print('REF_CLK_PD7', 'high', sum(bits), 'low', len(bits)-sum(bits),
              'transitions', sum(a != b for a, b in zip(bits, bits[1:])), flush=True)
        alternate(0x58020000, 2)
        alternate(0x58020800, 1)
        found = []
        for address in range(32):
            identifier = (mdio(address, 2), mdio(address, 3))
            if identifier[0] not in (0, 0xFFFF) and identifier[1] not in (0, 0xFFFF):
                found.append(address)
                mdio(address, 1)  # BMSR link is latch-low: read twice.
                status = mdio(address, 1)
                print('PHY', address, *(f'0x{v:04X}' for v in identifier),
                      'BMSR', hex(status), 'link', bool(status & 4),
                      'autoneg', bool(status & 32), flush=True)
        print('RESPONDING_PHYS', len(found), flush=True)
        if not found:
            raise RuntimeError('No PHY responded to the MDIO scan')
        if args.test == 'status':
            return
        if len(found) != 1 or mdio(found[0], 2) != 7 or mdio(found[0], 3) & 0xFFF0 != 0xC130:
            raise RuntimeError('Expected exactly one LAN8742A before PHY writes')
        address = found[0]
        control = mdio(address, 0)
        if args.test == 'restart':
            phy_write(address, 0, (control | 0x1200) & ~0x0C00)
            for i in range(6):
                time.sleep(1)
                mdio(address, 1)
                print('AUTONEG', i, hex(mdio(address, 1)), flush=True)
            return
        phy_saved = {r: mdio(address, r) for r in (0, 24, 25, 27)}
        try:
            for channel, mode in [('MDI', 0x8000), ('MDIX', 0xA000)]:
                phy_write(address, 27, mode)
                phy_write(address, 0, 0x2100)
                time.sleep(.05)
                for repeat in range(3):
                    phy_write(address, 25, 0x8000)
                    deadline = time.monotonic() + 2
                    while True:
                        result = mdio(address, 25)
                        if result & 0x100 and not result & 0x8000:
                            break
                        if time.monotonic() > deadline:
                            raise RuntimeError('TDR timeout')
                        time.sleep(.01)
                    print('TDR', channel, repeat, hex(result),
                          ['DEFAULT', 'SHORT', 'OPEN', 'MATCH'][(result >> 9) & 3],
                          'electrical_length', result & 255, flush=True)
        finally:
            for r in (25, 24, 27, 0):
                value = phy_saved[r]
                if r == 0 and value & 0x1000:
                    value |= 0x200
                phy_write(address, r, value)
    finally:
        try:
            for address, (mask, old) in reversed(list(saved.items())):
                write(address, (read(address) & ~mask) | (old & mask))
        finally:
            api.disconnect()
            dll_directory.close()


if __name__ == '__main__':
    main()

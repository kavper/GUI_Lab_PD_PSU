#!/usr/bin/env python3
"""Generate gui/src/common/ErrorCatalog.cpp from docs/errors/h7_errors_pl.json.

The JSON (from the G4/G0 firmware sources and the TI datasheets) is the
reference; the UI is English, so every symbol has a short English text here.
A symbol without a translation falls back to the symbol itself.
"""
import json, os, sys

EN = {
 'BMS.fault': {'CELL_OV': 'Cell overvoltage', 'CELL_UV': 'Cell undervoltage', 'PACK_OV': 'Pack overvoltage',
               'PACK_UV': 'Pack undervoltage', 'IMBALANCE': 'Cell imbalance ≥ 100 mV', 'SAFETY': 'BMS protection active',
               'ALERT': 'BMS event latched', 'I2C': 'G4–BMS link error', 'FET_START': 'Battery path start failed'},
 'BMS.sa': {'SCD': 'Short circuit in discharge', 'OCD2': 'Discharge overcurrent (2)', 'OCD1': 'Discharge overcurrent (1)',
            'OCC': 'Charge overcurrent', 'COV': 'Cell overvoltage', 'CUV': 'Cell undervoltage'},
 'BMS.sb': {'OTF': 'FET overtemperature', 'OTINT': 'BMS chip overtemperature', 'OTD': 'Too hot to discharge',
            'OTC': 'Too hot to charge', 'UTINT': 'BMS chip too cold', 'UTD': 'Too cold to discharge', 'UTC': 'Too cold to charge'},
 'BMS.sc': {'OCD3': 'Discharge overcurrent (3)', 'SCDL': 'Short-circuit latch', 'OCDL': 'Overcurrent latch',
            'COVL': 'Cell overvoltage latch', 'PTO': 'BMS precharge timeout', 'HWDF': 'BMS host watchdog'},
 'BMS.alarm': {'SSBC': 'Protection event (B/C)', 'SSA': 'Protection event (A)', 'PF': 'Permanent fail – service',
               'MSK_SFALERT': 'Safety alert (masked)', 'MSK_PFALERT': 'PF alert (masked)', 'INITSTART': 'Init started',
               'INITCOMP': 'Init complete', 'FULLSCAN': 'Full scan done', 'XCHG': 'Charging blocked',
               'XDSG': 'Discharging blocked', 'SHUTV': 'Shutdown voltage reached', 'FUSE': 'Fuse event',
               'CB': 'Cell balancing', 'ADSCAN': 'ADC scan done', 'WAKE': 'Wake-up'},
 'PD.bq_fault': {'ACOV': 'Charger input overvoltage', 'BATOC': 'Battery overcurrent', 'ACOC': 'Charger input overcurrent',
                 'SYSOVP': 'SYS overvoltage', 'VSYS_UVP': 'SYS undervoltage', 'FORCE_CONVERTER_OFF': 'Charger converter forced off',
                 'OTG_OVP': 'OTG output overvoltage', 'OTG_UVP': 'OTG output undervoltage'},
 'METER.psu_fault': {'DRIVER': 'Converter driver fault', 'OVP': 'Output overvoltage', 'OCP': 'Output overcurrent',
                     'UVIN': 'Converter input too low', 'ADC': 'ADC measurement fault', 'BMS': 'Output stopped by BMS'},
 'METER.g0_fault': {'HW_INIT': 'G0 hardware init failed', 'PGOOD_LOST': 'G0 power-good lost', 'POWER_KILL': 'G0 kill signal active',
                    'VIN_LOW': 'G0 input too low', 'VOUT_HARD': 'G0 output over limit (hard)', 'VOUT_HIGH': 'G0 output over limit',
                    'TEMP_HIGH': 'G0 overtemperature', 'IOUT_HARD': 'G0 overcurrent (hard)'},
 'NACK.reason': {'UNKNOWN': 'Unknown command', 'BAD_PAYLOAD': 'Malformed command', 'RANGE': 'Parameter out of range',
                 'UNSAFE': 'Not allowed in this state', 'BUSY': 'Device busy', 'TIMEOUT': 'Response timeout', 'LINK': 'Required link missing'},
 'AUX.stop_reason': {'NONE': 'No stop recorded', 'HOST_LINK': 'H7–G4 link lost', 'G0_LINK': 'G4–G0 link lost',
                     'G0_KILL': 'Stopped by G0 kill', 'G0_FAULT': 'Stopped by a G0 fault', 'START_FAILURE': 'Output start failed'},
 'AUX.sense_code': {'OK': 'Sense within limits', 'NOT_READY': 'Too low to test sense', 'DROP_P': '+ sense lead drop too high',
                    'DROP_SUM': 'Total sense lead drop too high', 'REVERSED': 'Sense leads reversed', 'N_ON_POUT': 'Both sense leads on +OUT',
                    'SHORT': 'Sense leads shorted?', 'DROP_N': '- sense lead drop too high', 'NO_SAMPLE': 'No sense ADC sample',
                    'LOCAL_HIGH': 'Local sense out of range', 'CV_VD': 'Remote sense disagrees in CV'},
 'BATT.soc_flags': {'VALID': 'SOC valid', 'UNCERTAIN': 'SOC not anchored yet', 'RESTING': 'At rest', 'RELAXED': 'Relaxed',
                    'FCC_LEARNED': 'Capacity learned', 'CHARGING': 'Charging', 'DISCHARGING': 'Discharging', 'FULL': 'Full (anchored)',
                    'LOW': 'Low charge', 'EMPTY': 'Pack empty', 'RESTORED': 'SOC restored', 'CLEAN_RESTORE': 'Clean restore', 'DIRTY': 'Save pending'},
 'BMS.st': {'ABSENT': 'BMS absent', 'INIT': 'BMS initialising', 'READY': 'BMS ready', 'DISCHARGING': 'Discharging',
            'CHARGING': 'Charging', 'WARN': 'BMS warning', 'FAULT': 'BMS fault'},
}
ENUM = {'BMS.fault': 'E_BMS_FAULT', 'BMS.sa': 'E_BMS_SA', 'BMS.sb': 'E_BMS_SB', 'BMS.sc': 'E_BMS_SC', 'BMS.alarm': 'E_BMS_ALARM',
        'PD.bq_fault': 'E_PD_BQ', 'METER.psu_fault': 'E_PSU', 'METER.g0_fault': 'E_G0', 'NACK.reason': 'E_NACK',
        'AUX.stop_reason': 'E_STOP', 'AUX.sense_code': 'E_SENSE', 'BATT.soc_flags': 'E_SOC', 'BMS.st': 'E_BMS_ST'}
SEV = {'info': 'ES_INFO', 'reserved': 'ES_RESERVED', 'warning': 'ES_WARNING', 'fault': 'ES_FAULT'}

def cstr(s):
    out, hexed = '"', False
    for c in s:
        if ord(c) >= 128:
            out += ''.join('\\x%02X' % b for b in c.encode()); hexed = True
            continue
        if hexed and c in '0123456789abcdefABCDEF':
            out += '" "'  # stop the hex escape from swallowing this digit
        hexed = False
        out += '\\' + c if c in '"\\' else c
    return out + '"'

src, out = sys.argv[1], sys.argv[2]
data = json.load(open(src, encoding='utf-8'))
lines = ['// Generated by tools/error_catalog.py from docs/errors/h7_errors_pl.json - do not edit.',
         '#include <gui/common/ErrorCatalog.hpp>', 'namespace err {']
order = []
for g in data['groups']:
    name = ENUM[g['key']]
    order.append((name, g))
    lines.append('static const Entry %s_entries[] = {' % name.lower())
    for e in g['entries']:
        text = EN[g['key']].get(e['symbol'], e['symbol'])
        lines.append('    { 0x%Xu, %s, %s, %s },' % (e['value'], cstr(e['symbol']), cstr(text), SEV[e['severity']]))
    lines.append('};')
lines.append('const Group groups[GROUP_COUNT] = {')
by = {n: g for n, g in order}
for n in ['E_BMS_FAULT', 'E_BMS_SA', 'E_BMS_SB', 'E_BMS_SC', 'E_BMS_ALARM', 'E_PD_BQ', 'E_PSU', 'E_G0', 'E_NACK', 'E_STOP', 'E_SENSE', 'E_SOC', 'E_BMS_ST']:
    g = by[n]
    lines.append('    { %s_entries, %d, %s },' % (n.lower(), len(g['entries']), 'true' if g['kind'] == 'bitmask' else 'false'))
lines.append('};')
lines.append('}')
open(out, 'w', encoding='utf-8').write('\n'.join(lines) + '\n')
print('groups', len(order))

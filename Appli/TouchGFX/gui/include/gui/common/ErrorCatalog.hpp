#ifndef ERRORCATALOG_HPP
#define ERRORCATALOG_HPP
// Plain-language meaning of the G4 / G0 / BMS / charger status codes.
// Generated table: tools/error_catalog.py (docs/errors/h7_errors_pl.json).
// Decode by group, never by the hex value alone: BMS.fault 0x10 is cell
// imbalance, BMS.sb 0x10 is charge overtemperature.
#include <stdint.h>
#include <stddef.h>

class TelemetryRecord;

namespace err {
enum GroupId { E_BMS_FAULT, E_BMS_SA, E_BMS_SB, E_BMS_SC, E_BMS_ALARM, E_PD_BQ, E_PSU, E_G0, E_NACK,
               E_STOP, E_SENSE, E_SOC, E_BMS_ST, GROUP_COUNT };
enum Severity { ES_INFO, ES_RESERVED, ES_WARNING, ES_FAULT };
struct Entry { uint32_t value; const char* symbol; const char* text; uint8_t severity; };
struct Group { const Entry* entries; uint8_t count; bool bitmask; };
extern const Group groups[GROUP_COUNT];

// Enum value (or a single bit) of a group; 0 when unknown.
const Entry* find(GroupId g, uint32_t value);
// Set bits of a mask at or above minSeverity, most severe first. Bits the
// table does not know are returned in *unknown.
unsigned active(GroupId g, uint32_t mask, const Entry** out, unsigned max, uint32_t* unknown = 0, uint8_t minSeverity = ES_WARNING);
// "OCC + COV" style list of symbols (short, for small boxes).
void symbols(GroupId g, uint32_t mask, char* out, size_t n, uint8_t minSeverity = ES_WARNING);

// Readable text for the H7 fault latch reason ("G4 FAULT 0x24" ->
// "Output overcurrent +1 more"); other reasons are returned unchanged.
const char* latchText(const char* reason, char* out, size_t n);
// Most important current issue of the BMS (TB record), e.g. "Cell imbalance
// >= 100 mV" or "Too hot to charge +1". Returns its severity, or -1 when
// nothing is wrong (out is then empty).
int bmsIssue(const TelemetryRecord& tb, char* out, size_t n);
// Same for the output path: G4 fault mask, G0 faults, stop reason.
int outputIssue(const TelemetryRecord& meter, const TelemetryRecord& aux, char* out, size_t n);
}
#endif

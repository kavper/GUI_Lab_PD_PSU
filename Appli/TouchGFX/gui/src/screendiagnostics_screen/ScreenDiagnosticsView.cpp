#include <gui/screendiagnostics_screen/ScreenDiagnosticsView.hpp>
#include <gui/common/LabText.hpp>
extern "C" {
#include "psu_app.h"
#include "psu_format.h"
#ifndef SIMULATOR
#include "ldo_protocol.h"
#endif
}

ScreenDiagnosticsView::ScreenDiagnosticsView()
    : refreshDivider(0)
{

}

void ScreenDiagnosticsView::setupScreen()
{
    ScreenDiagnosticsViewBase::setupScreen();
    handleTickEvent();
}

void ScreenDiagnosticsView::handleTickEvent()
{
    if (++refreshDivider < 10)
        return;
    refreshDivider = 0;

    {
        PsuSnapshot face;
        char reading[24];
        psu_app_ensure();
        psu_snapshot(&face);
        lab_show(DiagLink, DiagLinkBuffer, DIAGLINK_SIZE,
                 face.g0_connected ? (face.g0_stale ? "STALE" : "ONLINE") : "OFFLINE",
                 face.g0_connected ? (face.g0_stale ? lab_amber() : lab_green()) : lab_muted());
        psu_format_voltage(reading, sizeof(reading), face.vout_mv);
        lab_show(DiagVolt, DiagVoltBuffer, DIAGVOLT_SIZE, reading, lab_cyan());
        psu_format_current_ua(reading, sizeof(reading), face.display_current_ua);
        lab_show(DiagAmp, DiagAmpBuffer, DIAGAMP_SIZE, reading, face.mode_cc ? lab_amber() : lab_green());
        lab_show(DiagFault, DiagFaultBuffer, DIAGFAULT_SIZE,
                 face.fault_latched ? "FAULT" : "NONE",
                 face.fault_latched ? lab_red() : lab_green());
    }

#ifndef SIMULATOR
    LDO_Diagnostics diag;
    LDO_Telemetry telemetry;
    PsuSnapshot snap;
    const char *snap_g4;
    LDO_GetDiagnostics(&diag);
    LDO_GetTelemetry(&telemetry);
    psu_snapshot(&snap);
    snap_g4 = snap.g4_link == G4_LINK_ONLINE ? "ONLINE" :
              (snap.g4_link == G4_LINK_STALE ? "STALE" : "OFFLINE");
    touchgfx::Unicode::snprintf(
        DiagnosticsValuesBuffer, DIAGNOSTICSVALUES_SIZE,
        "LINK: %u   PROTOCOL: %u   TELEMETRY: %u   PERIOD: %u ms\n"
        "RX: %u B   VALID: %u   CRC ERR: %u\n"
        "TX: %u   ACK: %u   NACK: %u   TIMEOUT: %u   LAST NACK: %u\n"
        "UPTIME: %u ms\n"
        "STATUS: 0x%04X   FAULTS: 0x%08X\n"
        "VIN: %u mV   VOUT: %u mV   IOUT: %d uA\n"
        "IOUT ADC RAW: %d\n"
        "DAC CV: %u mV   DAC CC: %u mV\n"
        "REQUESTED: %u mV / %u mA\n"
        "APPLIED: %u mV / %u mA\n"
        "PREREGULATOR: %u mV\n"
        "T1 MOSFET: %d cC   T2 AMBIENT: %d cC\n"
        "T3 BLEEDER: %d cC   T4 PSU AREA: %d cC\n"
        "MODE: %u   STARTUP: %u   RESERVED: %u\n"
        "MAXIMUM: %u mV / %u mA\n"
        "CAPABILITIES: 0x%08X   CURRENT VALID: %u   CALIBRATED: %u\n"
        "PENDING TYPE: 0x%02X\nG4 %s",
        telemetry.connected ? 1U : 0U,
        telemetry.protocol_version, telemetry.telemetry_version,
        telemetry.telemetry_period_ms,
        (unsigned)diag.rx_bytes, (unsigned)diag.valid_frames,
        (unsigned)diag.crc_errors,
        (unsigned)diag.tx_frames,
        (unsigned)diag.ack_frames, (unsigned)diag.nack_frames,
        (unsigned)diag.command_timeouts, diag.last_nack_reason,
        (unsigned)telemetry.uptime_ms,
        telemetry.status_flags, (unsigned)telemetry.fault_flags,
        (unsigned)telemetry.vin_mv, (unsigned)telemetry.vout_mv,
        (int)telemetry.iout_ua, (int)telemetry.iout_adc_raw,
        (unsigned)telemetry.dac_cv_readback_mv,
        (unsigned)telemetry.dac_cc_readback_mv,
        (unsigned)telemetry.requested_voltage_mv,
        (unsigned)telemetry.requested_current_ma,
        (unsigned)telemetry.applied_voltage_mv,
        (unsigned)telemetry.applied_current_ma,
        (unsigned)telemetry.preregulator_mv,
        (int)telemetry.temperature_centi_c[0],
        (int)telemetry.temperature_centi_c[1],
        (int)telemetry.temperature_centi_c[2],
        (int)telemetry.temperature_centi_c[3],
        telemetry.mode, telemetry.startup, telemetry.reserved,
        (unsigned)telemetry.maximum_voltage_mv,
        (unsigned)telemetry.maximum_current_ma,
        (unsigned)telemetry.capability_flags,
        telemetry.current_valid, telemetry.current_calibrated,
        diag.pending_type,
        snap_g4);
#else
    char ascii[800];
    psu_render_diagnostics(ascii, sizeof(ascii));
    touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(ascii),
                               DiagnosticsValuesBuffer, DIAGNOSTICSVALUES_SIZE);
#endif
    DiagnosticsValues.invalidate();
}

void ScreenDiagnosticsView::tearDownScreen()
{
    ScreenDiagnosticsViewBase::tearDownScreen();
}

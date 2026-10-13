#include <gui/common/BatteryTrend.hpp>
#include <gui/common/TelemetryData.hpp>
BatteryTrend& batteryTrend(){static BatteryTrend history;return history;}
void updateBatteryTrend(){
    TelemetryRecord pack(G4_RECORD_TB);int64_t mv=0,ma=0;
    bool valid=pack.is("bms",1)&&pack.is("sample",1)&&pack.get("pack_mv",mv)&&pack.get("i_pack_ma",ma)&&
        mv>=0&&mv<=100000&&ma>=-100000&&ma<=100000;
    batteryTrend().update(valid,pack.record.ms,valid?(int32_t)(mv*ma/1000):0);
}

#include <gui/common/PsuConfigStore.hpp>

static PsuPreset presets[3] = {{5000,1000},{12000,2000},{20000,3000}};
static PsuSequenceStep steps[12] = {
    {5000,1000,120,1000,true},
    {12000,2000,300,500,true},
    {20000,3000,180,1000,true},
    {0,0,60,0,false},
    {5000,1000,120,1000,true},{5000,1000,120,1000,true},
    {5000,1000,120,1000,true},{5000,1000,120,1000,true},
    {5000,1000,120,1000,true},{5000,1000,120,1000,true},
    {5000,1000,120,1000,true},{5000,1000,120,1000,true}
};
static uint8_t sequenceStepCount = 1;
static uint8_t sequenceLoopMode = 1;
static uint32_t sequenceConfigRevision = 1;
static uint32_t sequenceRuntimeRevision = 1;
static uint8_t sequenceCommand = 0;
static PsuSequenceRuntime sequenceRuntime = {false, 0, 1, 0, 0, 0, PSU_SEQUENCE_IDLE};
static uint32_t mainVoltageMv = 12000;
static uint32_t mainCurrentMa = 2000;
static uint32_t mainSetpointRevision = 1;
static bool sequenceAbortRequested = false;
static bool sequenceRunning = false;

PsuPreset& psuPreset(uint8_t index) { return presets[index < 3 ? index : 0]; }
PsuSequenceStep& psuSequenceStep(uint8_t index) { return steps[index < 12 ? index : 0]; }
uint8_t psuSequenceStepCount() { return sequenceStepCount; }
void psuSetSequenceStepCount(uint8_t count) { count=count<1?1:(count>12?12:count); if(sequenceStepCount!=count){sequenceStepCount=count;++sequenceConfigRevision;} }
void psuTouchSequenceConfig() { ++sequenceConfigRevision; }
uint32_t psuSequenceConfigRevision() { return sequenceConfigRevision; }
uint8_t psuSequenceLoopMode() { return sequenceLoopMode; }
void psuSetSequenceLoopMode(uint8_t mode) { if(mode!=1&&mode!=2&&mode!=3&&mode!=5&&mode!=255)mode=1;if(sequenceLoopMode!=mode){sequenceLoopMode=mode;++sequenceConfigRevision;} }
void psuRequestSequenceStart() { sequenceCommand=1; }
void psuRequestSequenceStop() { sequenceCommand=2; }
uint8_t psuConsumeSequenceCommand() { const uint8_t command=sequenceCommand;sequenceCommand=0;return command; }
void psuSetSequenceRuntime(const PsuSequenceRuntime& runtime)
{
    const bool visibleChange=sequenceRuntime.running!=runtime.running || sequenceRuntime.runningStep!=runtime.runningStep ||
       sequenceRuntime.loopMode!=runtime.loopMode || sequenceRuntime.completedLoops!=runtime.completedLoops ||
       sequenceRuntime.state!=runtime.state;
    sequenceRuntime=runtime;if(visibleChange)++sequenceRuntimeRevision;sequenceRunning=runtime.running;
}
void psuGetSequenceRuntime(PsuSequenceRuntime& runtime) { runtime=sequenceRuntime; }
uint32_t psuSequenceRuntimeRevision() { return sequenceRuntimeRevision; }
void psuSetMainSetpoints(uint32_t voltageMv, uint32_t currentMa)
{
    if (voltageMv > 27000U) voltageMv = 27000U;
    if (currentMa > 5000U) currentMa = 5000U;
    if (mainVoltageMv == voltageMv && mainCurrentMa == currentMa) return;
    mainVoltageMv=voltageMv; mainCurrentMa=currentMa; ++mainSetpointRevision;
}
void psuGetMainSetpoints(uint32_t& voltageMv, uint32_t& currentMa) { voltageMv=mainVoltageMv; currentMa=mainCurrentMa; }
uint32_t psuMainSetpointRevision() { return mainSetpointRevision; }
void psuRequestSequenceAbort() { sequenceAbortRequested=true; }
bool psuConsumeSequenceAbort() { const bool requested=sequenceAbortRequested; sequenceAbortRequested=false; return requested; }
void psuSetSequenceRunning(bool running) { sequenceRunning=running; }
bool psuIsSequenceRunning() { return sequenceRunning; }

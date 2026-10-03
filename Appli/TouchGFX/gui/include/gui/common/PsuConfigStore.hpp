#ifndef PSUCONFIGSTORE_HPP
#define PSUCONFIGSTORE_HPP

#include <stdint.h>

struct PsuPreset
{
    uint32_t voltageMv;
    uint32_t currentMa;
};

struct PsuSequenceStep
{
    uint32_t voltageMv;
    uint32_t currentMa;
    uint32_t durationTicks; // TouchGFX ticks, nominally 60 Hz
    uint32_t slewMvPerSecond; // 0 means immediate
    bool enabled;
};

enum PsuSequenceState
{
    PSU_SEQUENCE_IDLE = 0,
    PSU_SEQUENCE_RUNNING,
    PSU_SEQUENCE_COMPLETE,
    PSU_SEQUENCE_STOPPED,
    PSU_SEQUENCE_NO_ENABLED_STEPS,
    PSU_SEQUENCE_ABORTED
};

struct PsuSequenceRuntime
{
    bool running;
    uint8_t runningStep;
    uint8_t loopMode;
    uint8_t completedLoops;
    uint32_t stepTicks;
    uint32_t commandedMv;
    PsuSequenceState state;
};

PsuPreset& psuPreset(uint8_t index);
PsuSequenceStep& psuSequenceStep(uint8_t index);
uint8_t psuSequenceStepCount();
void psuSetSequenceStepCount(uint8_t count);
void psuTouchSequenceConfig();
uint32_t psuSequenceConfigRevision();
uint8_t psuSequenceLoopMode();
void psuSetSequenceLoopMode(uint8_t mode);
void psuRequestSequenceStart();
void psuRequestSequenceStop();
uint8_t psuConsumeSequenceCommand(); // 1=start, 2=stop
void psuSetSequenceRuntime(const PsuSequenceRuntime& runtime);
void psuGetSequenceRuntime(PsuSequenceRuntime& runtime);
uint32_t psuSequenceRuntimeRevision();
void psuSetMainSetpoints(uint32_t voltageMv, uint32_t currentMa);
void psuGetMainSetpoints(uint32_t& voltageMv, uint32_t& currentMa);
uint32_t psuMainSetpointRevision();
void psuRequestSequenceAbort();
bool psuConsumeSequenceAbort();
void psuSetSequenceRunning(bool running);
bool psuIsSequenceRunning();

#endif

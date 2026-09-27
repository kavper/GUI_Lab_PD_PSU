#ifndef PSU_SEQ_H
#define PSU_SEQ_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PSU_SEQ_MAX_STEPS 12u

typedef enum
{
  PSU_SEQ_ONCE = 0,
  PSU_SEQ_N = 1,
  PSU_SEQ_INFINITE = 2
} PsuSeqMode;

typedef enum
{
  PSU_SEQ_IDLE = 0,
  PSU_SEQ_RUN = 1,
  PSU_SEQ_PAUSE = 2,
  PSU_SEQ_DONE = 3,
  PSU_SEQ_ABORTED = 4
} PsuSeqRun;

typedef enum
{
  PSU_STEP_KEEP = 0,
  PSU_STEP_ON = 1,
  PSU_STEP_OFF = 2
} PsuStepOutput;

typedef enum
{
  PSU_STOP_OFF = 0,
  PSU_STOP_HOLD = 1,
  PSU_STOP_START = 2
} PsuStopAction;

typedef struct
{
  uint32_t voltage_mv;
  uint32_t current_ma;
  uint32_t time_ms;
  uint32_t slew_mv_per_s; /* 0 = immediate, else 1..100000 */
  uint8_t enabled;
  uint8_t output_action;
} PsuSeqStep;

typedef struct
{
  void (*limits)(uint32_t mv, uint32_t ma, void *user);
  void (*output)(int on, void *user);
  int (*applied)(uint32_t *mv, uint32_t *ma, void *user);
  int (*permit_ok)(void *user);
  void *user;
} PsuSeqIo;

typedef struct
{
  PsuSeqStep steps[PSU_SEQ_MAX_STEPS];
  uint8_t count;
  uint8_t selected;
  uint8_t mode;
  uint16_t loops_requested;
  uint8_t stop_action;
  uint8_t run;
  uint8_t step_index;
  uint16_t loop_index;
  uint32_t step_started_ms;
  uint32_t seq_started_ms;
  uint32_t pause_accum_ms;
  uint32_t ramp_mv;
  uint32_t command_ms;
  uint8_t waiting_readback;
  uint8_t config_locked;
  char status[48];
  PsuSeqIo io;
} PsuSequencer;

void psu_seq_init(PsuSequencer *seq);
int psu_seq_add(PsuSequencer *seq);
int psu_seq_remove_selected(PsuSequencer *seq);
int psu_seq_set_step(PsuSequencer *seq, uint8_t index, const PsuSeqStep *step);
int psu_seq_start(PsuSequencer *seq, uint32_t now_ms);
void psu_seq_pause(PsuSequencer *seq, uint32_t now_ms);
void psu_seq_resume(PsuSequencer *seq, uint32_t now_ms);
void psu_seq_stop(PsuSequencer *seq, uint32_t now_ms, int abort_timeout);
void psu_seq_tick(PsuSequencer *seq, uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif

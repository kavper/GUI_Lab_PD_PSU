#ifndef G4_ASCII_H
#define G4_ASCII_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Legacy filename retained for existing Designer/IDE source lists.
   Production UART7 is binary at 460800 8N1; text is diagnostic TEXT only. */

#define G4_LINE_MAX 96
#define G4_RX_LINE_MAX 1536
#define G4_QUEUE_LEN 10
#define G4_RAW_LOG 12
#define G4_SETPOINT_GAP_MS 0u
#define G4_CMD_TIMEOUT_MS 800u

enum { G4_OUTPUT_IDLE, G4_OUTPUT_STARTING, G4_OUTPUT_RUNNING, G4_OUTPUT_STOPPING };
enum { G4_G0_POWER_KILL = 1u << 2, G4_G0_VIN_LOW = 1u << 3,
       G4_G0_MEAS_LOST = 1u << 8 };

/* Named numeric telemetry, including optional fields. Missing is not zero. */
typedef struct {
  int64_t values[64];
  uint64_t present;
  uint32_t ms;
  uint8_t kind, valid;
} G4Record;
enum { G4_RECORD_T, G4_RECORD_TB, G4_RECORD_TC, G4_RECORD_AUX };

typedef struct {
  uint8_t valid, g0, out, want, mode, permit, remote;
  uint32_t ms, serial, vin_mv, rail_mv, set_mv, ilim_ma, vout_mv;
  int32_t iout_ma;
  uint32_t fault, g0_fault, g0_age_ms, g0_rx, g0_tlm, g0_err, g0_uart;
  uint32_t vpre_req_mv, vpre_cmd_mv, pd_mv, pd_ma;
  uint32_t g0_vin_mv, g0_vset_mv, g0_iset_ma;
  uint8_t pd;
  uint8_t run, stage_en, ps_en, kill, ctrl, fault_latch;
} G4Telemetry;
typedef struct {
  uint8_t valid, present, configured, sample, chg, dsg, series;
  uint32_t ms, fault, pack_mv, stack_mv;
  int32_t cell_mv[5], current_ma;
} G4Battery;
typedef struct {
  uint8_t valid, online, pd_role, plug, otg;
  uint32_t ms, vbat_mv, vsys_mv, vbus_mv, pd_mv, pd_ma, fault;
  int32_t ibat_ma;
} G4Charger;

typedef enum
{
  G4_LINK_OFFLINE = 0,
  G4_LINK_ONLINE = 1,
  G4_LINK_STALE = 2
} G4Link;

typedef struct
{
  char text[40];
  uint8_t safety;
  uint8_t kind; /* 1 SET, 2 ILIM, 3 PPS, 0 other */
  uint16_t cmd_id;
} G4Slot;

typedef struct
{
  char text[G4_LINE_MAX];
  uint8_t dir_tx;
  uint32_t ms;
} G4RawLine;

typedef struct
{
  uint8_t raw_payload[4][72], raw_len[4];
  uint16_t transaction_id[256];
  uint8_t transaction_type[256], transaction_active[256];
  uint32_t transaction_ms[256];
  uint16_t replies_id[16];
  uint8_t replies_state[16], reply_head, reply_tail, reply_count;
  uint32_t parser_ms;
  uint8_t frame[120], frame_len;
  uint8_t wire_queue[10][120], wire_len[10], wire_seq[10];
  uint16_t wire_id[10];
  uint8_t pending_frame[120], pending_len, pending_seq;
  uint8_t off_frame[120], off_len;
  uint8_t safety_frames[8][120], safety_lengths[8], safety_head, safety_tail, safety_count;
  uint16_t safety_ids[8];
  uint16_t off_id, pending_id;
  uint8_t inflight[2][120], inflight_len[2], inflight_seq[2];
  uint16_t inflight_id[2];
  uint32_t inflight_ms[2];
  uint8_t sequence, nack_reason, rx_seq[4], rx_seen[4];
  G4Slot queue[G4_QUEUE_LEN];
  uint8_t head;
  uint8_t tail;
  uint8_t count;
  G4Slot safety;
  uint8_t safety_pending;
  char pending_set[40];
  char pending_ilim[40];
  char pending_pps[40];
  uint8_t has_set;
  uint8_t has_ilim;
  uint8_t has_pps;
  uint16_t set_id;
  uint16_t ilim_id;
  uint16_t pps_id;
  uint32_t last_pair_ms;
  uint8_t pair_open;
  uint8_t need_session_init;
  uint8_t session_ready;
  uint8_t init_phase;
  G4Link link;
  uint32_t last_rx_ms;
  uint32_t tx_count;
  uint32_t rx_bytes;
  uint32_t rx_lines;
  uint32_t overflow_count;
  uint32_t parse_error_count;
  char rx_acc[G4_RX_LINE_MAX];
  uint16_t rx_len;
  uint8_t rx_dropping, seen_rx;
  G4Telemetry telemetry;
  G4Battery battery;
  G4Charger charger;
  G4Record records[4];

  uint32_t ack_count, err_count, event_fault, event_g0_fault;
  uint8_t event_ctrl_fault, event_g0_stale, output_phase;
  G4Telemetry event_meter;
  uint8_t event_phase;
  uint8_t nack_valid, nack_type, nack_seq, nack_matched;
  uint32_t nack_ms;
  uint16_t active_id, response_id, last_tx_id;
  uint8_t response_state, awaiting, retry_used;
  uint32_t active_ms, set_due_ms;
  char active_cmd[40], retry_set[40], last_rx[G4_LINE_MAX];
  G4RawLine log[G4_RAW_LOG];
  uint8_t log_count;
  uint8_t log_pos;
  char last_tx[G4_LINE_MAX];
  uint16_t next_id;
  uint8_t pps_ctl;
  uint8_t sink_role;
  uint8_t apdo_selected;
  uint8_t service_mode;
} G4Port;

uint16_t g4_crc16(const uint8_t *data, size_t n);
size_t g4_frame(uint8_t *out,uint8_t type,uint8_t seq,const uint8_t *payload,size_t n);
int g4_pop_frame(G4Port *port,uint8_t *out,size_t n,uint32_t now_ms);
void g4_reset_parser(G4Port *port);
int g4_response(G4Port *port,uint16_t *id,uint8_t *state);
/* Fault masks remain separate; OFF / early startup exceptions are contextual. */
uint32_t g4_blocking_g0_faults(const G4Telemetry *t, uint8_t phase);
int g4_start_wait(const G4Telemetry *t, uint8_t phase);
void g4_init(G4Port *port);
void g4_link_up(G4Port *port);
void g4_link_lost(G4Port *port);
int g4_enqueue(G4Port *port, const char *cmd, int safety, uint16_t cmd_id);
int g4_set_mv(G4Port *port, uint32_t voltage_mv, uint16_t cmd_id);
int g4_set_limits(G4Port *port, uint32_t voltage_mv, uint32_t current_ma, uint16_t cmd_id);
int g4_ilim_ma(G4Port *port, uint32_t current_ma, uint16_t cmd_id);
int g4_off(G4Port *port, uint16_t cmd_id);
int g4_power_shutdown(G4Port *port,uint16_t cmd_id);
int g4_on(G4Port *port, uint16_t cmd_id);
int g4_usb_role(G4Port *port, const char *role, uint16_t cmd_id);
int g4_pps(G4Port *port, uint32_t voltage_mv, uint32_t current_ma, uint16_t cmd_id);
int g4_permit(G4Port *port, int enabled, uint16_t cmd_id);
int g4_remote(G4Port *port, int enabled, uint16_t cmd_id);
int g4_verbose(G4Port *port, int enabled, uint16_t cmd_id);
int g4_simple(G4Port *port, const char *cmd, uint16_t cmd_id);
int g4_service_line(G4Port *port, const char *cmd, uint16_t cmd_id);
void g4_process(G4Port *port, uint32_t now_ms);
int g4_pop_tx(G4Port *port, char *out, size_t n, uint32_t now_ms);
void g4_rx_bytes(G4Port *port, const uint8_t *data, size_t n, uint32_t now_ms);
void g4_set_pps_gates(G4Port *port, int pps_ctl, int sink, int apdo);
int g4_pps_allowed(const G4Port *port);
int g4_record_value(const G4Record *record, const char *key, int64_t *out);
void g4_record_snapshot(const G4Port *port, uint8_t kind, G4Record *out);
void g4_cancel_pending(G4Port *port);
const char *g4_record_key(uint8_t kind, unsigned index);
void g4_raw_snapshot(const G4Port *port, uint8_t kind, char *out, size_t n);

#ifdef __cplusplus
}
#endif

#endif

#ifndef PSU_APP_H
#define PSU_APP_H

#include "g4_ascii.h"
#include "psu_charger.h"
#include "psu_seq.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PSU_PRESET_COUNT 8u
#define PSU_CMD_LOG 8u

typedef enum
{
  PSU_SRC_BOOT = 0,
  PSU_SRC_LCD,
  PSU_SRC_WEB,
  PSU_SRC_SEQ,
  PSU_SRC_BUTTON,
  PSU_SRC_CHARGER
} PsuSource;

typedef enum
{
  PSU_CMD_QUEUED = 0,
  PSU_CMD_SENT,
  PSU_CMD_ACK,
  PSU_CMD_REJECTED,
  PSU_CMD_TIMEOUT
} PsuCmdState;

typedef enum
{
  PSU_NET_DOWN = 0,
  PSU_NET_DHCP,
  PSU_NET_BOUND,
  PSU_NET_TIMEOUT
} PsuNetState;

typedef struct
{
  char name[16];
  uint32_t voltage_mv;
  uint32_t current_ma;
  uint32_t slew_mv_per_s;
  uint8_t output_action;
} PsuPreset;

typedef struct
{
  uint8_t connected;
  uint8_t stale;
  uint8_t current_valid;
  uint8_t current_calibrated;
  uint16_t status_flags;
  uint32_t fault_flags;
  uint32_t vin_mv;
  uint32_t vout_mv;
  int32_t iout_ua;
  int32_t iout_adc_raw;
  int16_t temp_centi_c[4];
  uint32_t applied_voltage_mv;
  uint32_t applied_current_ma;
  uint32_t maximum_voltage_mv;
  uint32_t maximum_current_ma;
  uint8_t mode;
  uint32_t uptime_ms;
  uint16_t period_ms;
  uint8_t protocol_version;
  uint8_t telemetry_version;
  uint32_t rx_bytes;
  uint32_t valid_frames;
  uint32_t crc_errors;
  uint32_t tx_frames;
  uint32_t ack_frames;
  uint32_t nack_frames;
  uint32_t timeouts;
} PsuG0Sample;

typedef struct
{
  uint32_t revision;
  uint32_t requested_mv;
  uint32_t requested_ma;
  uint32_t applied_mv;
  uint32_t applied_ma;
  uint8_t applied_valid;
  uint8_t output_requested;
  uint8_t output_confirmed;
  uint32_t power_mw;
  int32_t display_current_ua;
  int32_t signed_current_ua;
  uint8_t g0_connected;
  uint8_t g0_stale;
  uint8_t current_valid;
  uint8_t mode_cc;
  uint32_t vin_mv;
  uint32_t vout_mv;
  int16_t mos_centi;
  int16_t pcb_centi;
  uint8_t g4_link;
  uint8_t g4_uart_configured;
  uint8_t pps_allowed;
  uint8_t ethernet_configured;
  uint8_t net_state;
  uint8_t ip[4];
  uint8_t mask[4];
  uint8_t gateway[4];
  uint8_t mac[6];
  uint8_t fault_latched;
  char fault[40];
  uint16_t last_cmd_id;
  uint8_t last_cmd_state;
  uint8_t cold_output_off;
  uint8_t seq_run;
  uint8_t seq_count;
  uint8_t seq_step;
  uint32_t seq_mv;
  uint32_t seq_ma;
  char seq_status[48];
  uint8_t chg_state;
  char chg_reason[64];
  uint8_t bms_valid;
  uint16_t cell_mv[4];
  int16_t bms_temp_centi[3];
  uint8_t bms_fet_chg;
  uint8_t bms_fet_dsg;
  uint8_t bms_balancing;
  char bms_fault[40];
  uint8_t usb_role;
  uint8_t service_mode;
  char g4_last_tx[48];
  char g4_last_rx[48];
  uint32_t g0_crc_errors;
  uint32_t g4_overflow;
  uint8_t preset_selected;
} PsuSnapshot;

typedef struct
{
  void (*ldo_limits)(uint32_t voltage_mv, uint32_t current_ma);
  void (*ldo_output)(uint8_t enabled);
} PsuHalHooks;

void psu_app_init(void);
void psu_app_set_hooks(const PsuHalHooks *hooks);
void psu_app_tick(uint32_t now_ms);
void psu_app_observe_g0(const PsuG0Sample *sample, uint32_t now_ms);
void psu_app_user_button(int level_high, uint32_t now_ms);
int psu_app_set_limits(uint32_t voltage_mv, uint32_t current_ma, uint8_t source);
int psu_app_set_output(int enabled, uint8_t source);
int psu_app_usb_role(const char *role, uint8_t source);
int psu_app_service(int enabled);
int psu_app_service_cmd(const char *line);
int psu_app_clear_fault(void);
int psu_app_bms_cmd(const char *cmd);
void psu_snapshot(PsuSnapshot *out);

int psu_preset_apply(uint8_t index);
int psu_preset_save_current(uint8_t index, const char *name);
int psu_preset_duplicate(uint8_t from, uint8_t to);
void psu_preset_reset_defaults(void);
const PsuPreset *psu_preset_get(uint8_t index);

PsuSequencer *psu_sequencer(void);
PsuCharger *psu_charger(void);
G4Port *psu_g4(void);

void psu_net_down(void);
void psu_net_up(void);
int psu_net_bound(const uint8_t ip[4], const uint8_t mask[4], const uint8_t gw[4]);
void psu_net_dhcp_timeout(void);
void psu_net_set_mac(const uint8_t mac[6]);

int psu_config_save(void);
int psu_config_load(void);
void psu_config_bind_ram(uint8_t *slot_a, uint8_t *slot_b, size_t bytes);

int psu_web_write_state(char *dst, size_t n);
int psu_web_request(const char *method, const char *path, const char *body,
                    const char *client, char *dst, size_t n);
const char *psu_web_page(void);
/* Copy the next TCP-window sized piece. Returns bytes copied, 0 when done. */
size_t psu_http_next(const char *body, size_t len, size_t *offset, char *dst, size_t window);

void psu_render_diagnostics(char *dst, size_t n);
void psu_render_bms(char *dst, size_t n);
void psu_render_battery(char *dst, size_t n);
void psu_render_usb(char *dst, size_t n);
void psu_render_measurements(char *dst, size_t n);
void psu_render_protection(char *dst, size_t n);
void psu_render_network(char *dst, size_t n);
void psu_render_sequencer(char *dst, size_t n);
void psu_render_charger(char *dst, size_t n);
void psu_render_presets(char *dst, size_t n);
void psu_render_service(char *dst, size_t n);
void psu_render_power_path(char *dst, size_t n);

#if defined(SIMULATOR) || defined(PSU_SIMULATOR)
void psu_sim_tick(uint32_t now_ms);
#endif

#ifdef __cplusplus
}
#endif

#endif

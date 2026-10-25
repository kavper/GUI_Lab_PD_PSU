#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int psu_web_features_state(const char *path,char *out,size_t size);
int psu_web_features_submit(const char *body,char *out,size_t size);
void psu_web_features_init(void);
void psu_web_features_poll(void);
/* Consumed on the GUI thread only: bit 0 appearance apply, bit 1 buzzer test. */
unsigned psu_web_features_gui_events(void);
#ifdef __cplusplus
}
#endif

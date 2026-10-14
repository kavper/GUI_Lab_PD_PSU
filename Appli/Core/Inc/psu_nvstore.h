#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define PSU_NV_PAYLOAD 512u
#define PSU_NV_RECORD (20u + PSU_NV_PAYLOAD)
#define PSU_NV_BANK_SIZE 4096u
/* I/O: 1 success, 0 temporarily busy, -1 failure. Writes never cross a 32B page. */
typedef int (*PsuNvIo)(uint16_t address, uint8_t *bytes, uint16_t length, void *user);
typedef struct {
    PsuNvIo read, write;
    void *user;
    uint8_t desired[PSU_NV_PAYLOAD], saved[PSU_NV_PAYLOAD];
    uint8_t record[PSU_NV_RECORD], verify[PSU_NV_RECORD];
    uint32_t generation, commits, errors, due, first_dirty, changed_at, last_commit;
    uint16_t offset;
    uint8_t active, target, state, dirty, loaded, failed, ready;
} PsuNvStore;
void psu_nv_init(PsuNvStore *s, PsuNvIo read, PsuNvIo write, void *user);
/* 1 valid record; 0 readable blank/invalid; -1 I/O failure (writes inhibited). */
int psu_nv_load(PsuNvStore *s, uint8_t *payload);
void psu_nv_request(PsuNvStore *s, const uint8_t *payload, uint32_t now);
void psu_nv_tick(PsuNvStore *s, uint32_t now);
#ifdef __cplusplus
}
#endif

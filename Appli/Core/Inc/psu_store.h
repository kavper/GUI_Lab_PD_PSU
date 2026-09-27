#ifndef PSU_STORE_H
#define PSU_STORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PSU_STORE_MAGIC 0x31555350u /* 'PSU1' */
#define PSU_STORE_SCHEMA 1u
#define PSU_STORE_SLOT 4096u

typedef struct
{
  uint8_t *bytes;
  size_t size;
} PsuStoreSlot;

typedef struct
{
  PsuStoreSlot a;
  PsuStoreSlot b;
  uint32_t commits;
  int hw_program_enabled;
} PsuStore;

uint32_t psu_crc32(const uint8_t *data, size_t len);
void psu_store_init(PsuStore *store, uint8_t *a, uint8_t *b, size_t size);
int psu_store_save(PsuStore *store, const void *payload, uint16_t len);
int psu_store_load(PsuStore *store, void *payload, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif

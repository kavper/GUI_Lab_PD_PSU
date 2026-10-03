#include "psu_store.h"
#include <string.h>

uint32_t psu_crc32(const uint8_t *data, size_t len)
{
  uint32_t crc = 0xFFFFFFFFU;
  size_t i;
  int bit;
  for (i = 0; i < len; ++i)
  {
    crc ^= data[i];
    for (bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xEDB88320U & (uint32_t)-(int)(crc & 1U));
  }
  return ~crc;
}

void psu_store_init(PsuStore *store, uint8_t *a, uint8_t *b, size_t size)
{
  if (store == 0)
    return;
  memset(store, 0, sizeof(*store));
  store->a.bytes = a;
  store->a.size = size;
  store->b.bytes = b;
  store->b.size = size;
  store->hw_program_enabled = 1;
}

static int slot_ok(const PsuStoreSlot *slot, uint16_t expect, uint32_t *gen)
{
  uint32_t magic, generation, crc, calc;
  uint16_t schema, length;
  if (slot == 0 || slot->bytes == 0 || slot->size < 16U + expect)
    return 0;
  memcpy(&magic, slot->bytes, 4);
  memcpy(&schema, slot->bytes + 4, 2);
  memcpy(&length, slot->bytes + 6, 2);
  memcpy(&generation, slot->bytes + 8, 4);
  memcpy(&crc, slot->bytes + 12, 4);
  if (magic != PSU_STORE_MAGIC || schema != PSU_STORE_SCHEMA || length != expect)
    return 0;
  calc = psu_crc32(slot->bytes + 16, length);
  if (calc != crc)
    return 0;
  if (gen)
    *gen = generation;
  return 1;
}

int psu_store_load(PsuStore *store, void *payload, uint16_t len)
{
  uint32_t gen_a = 0, gen_b = 0;
  int ok_a, ok_b;
  const PsuStoreSlot *src;
  if (store == 0 || payload == 0 || len == 0U)
    return 0;
  ok_a = slot_ok(&store->a, len, &gen_a);
  ok_b = slot_ok(&store->b, len, &gen_b);
  if (!ok_a && !ok_b)
    return 0;
  if (ok_a && (!ok_b || gen_a >= gen_b))
    src = &store->a;
  else
    src = &store->b;
  memcpy(payload, src->bytes + 16, len);
  return 1;
}

int psu_store_save(PsuStore *store, const void *payload, uint16_t len)
{
  uint32_t gen_a = 0, gen_b = 0;
  int ok_a, ok_b;
  uint32_t next_gen = 1U;
  PsuStoreSlot *dst;
  uint32_t magic = PSU_STORE_MAGIC;
  uint16_t schema = PSU_STORE_SCHEMA;
  uint32_t crc;
  if (store == 0 || payload == 0 || !store->hw_program_enabled)
    return 0;
  if (store->a.bytes == 0 || store->b.bytes == 0)
    return 0;
  if ((size_t)len + 16U > store->a.size)
    return 0;
  ok_a = slot_ok(&store->a, len, &gen_a);
  ok_b = slot_ok(&store->b, len, &gen_b);
  if (ok_a && gen_a >= next_gen)
    next_gen = gen_a + 1U;
  if (ok_b && gen_b >= next_gen)
    next_gen = gen_b + 1U;
  /* Write the inactive (older or invalid) copy. */
  if (!ok_a)
    dst = &store->a;
  else if (!ok_b)
    dst = &store->b;
  else
    dst = (gen_a <= gen_b) ? &store->a : &store->b;
  memset(dst->bytes, 0, dst->size);
  memcpy(dst->bytes + 16, payload, len);
  crc = psu_crc32(dst->bytes + 16, len);
  memcpy(dst->bytes, &magic, 4);
  memcpy(dst->bytes + 4, &schema, 2);
  memcpy(dst->bytes + 6, &len, 2);
  memcpy(dst->bytes + 8, &next_gen, 4);
  memcpy(dst->bytes + 12, &crc, 4);
  ++store->commits;
  return 1;
}

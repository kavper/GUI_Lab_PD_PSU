#pragma once
#include <stdint.h>
typedef struct {
    uint32_t ms, attach_generation, rdo, active_pdo, pdo[7];
    uint8_t valid, flags, role, count, sequence, seen;
} G4PdCapabilities;

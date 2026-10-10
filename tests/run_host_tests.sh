#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
test_output=$(mktemp -d "${TMPDIR:-/tmp}/h7-host-tests.XXXXXX")
trap 'rm -rf "$test_output"' EXIT
units=(g4_ascii psu_app psu_sim psu_limits psu_store psu_seq psu_charger psu_format psu_edit)
for test_name in test_binary test_binary_app test_startup test_automation; do
  sources=()
  for unit in "${units[@]}"; do
    sources+=("Appli/Core/Src/$unit.c")
    if [[ "$test_name" == test_binary ]]; then break; fi
  done
  "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -DPSU_SIMULATOR \
    -IAppli/Core/Inc "tests/$test_name.c" "${sources[@]}" -lm \
    -o "$test_output/$test_name"
  "$test_output/$test_name"
done
for test_name in test_display_voltage test_power_gauge; do
  "${CXX:-c++}" -std=c++11 -Wall -Wextra -Werror \
    -IAppli/TouchGFX/gui/include "tests/$test_name.cpp" -o "$test_output/$test_name"
  "$test_output/$test_name"
done

#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
test_output=$(mktemp -d "${TMPDIR:-/tmp}/h7-host-tests.XXXXXX")
trap 'rm -rf "$test_output"' EXIT
units=(g4_ascii psu_app psu_sim psu_limits psu_store psu_nvstore psu_seq psu_charger psu_format psu_edit)
for test_name in test_binary test_binary_app test_startup test_automation test_nvstore test_pd_caps; do
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
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -IAppli/Core/Inc \
  tests/test_panel_input.c Appli/Core/Src/panel_input.c -o "$test_output/test_panel_input"
"$test_output/test_panel_input"
for test_name in test_display_voltage test_power_gauge test_chart_axis test_usb_session test_battery_trend test_rounded_corners test_pd_objects test_edge_back; do
  "${CXX:-c++}" -std=c++11 -Wall -Wextra -Werror \
    -IAppli/TouchGFX/gui/include -IAppli/Core/Inc "tests/$test_name.cpp" -o "$test_output/$test_name"
  "$test_output/$test_name"
done

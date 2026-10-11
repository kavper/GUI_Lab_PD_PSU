# Stable GUI with main REMOTE SENSE button

Base: 4f17628, before the task-focused GUI redesign. Only the main-screen SENSE control and the space needed by its three narrower presets are added. Core firmware, application data structures, background processing, touch driver and animation logic are unchanged. Existing BMS telemetry still contains the five physical channels.

SENSE displays actual AUX flags (LOCAL, ARMED, REMOTE, stale or fault). Switching uses the existing g4_remote command, also used by the original power-path screen. A fault opens that existing diagnostics screen. G4 is unchanged.

Designer Generate, simulator and target compilation passed. Simulator test passes four complete swipe routes; its cancelled-drag/editor-preservation assertion fails with the unchanged old selection logic, which this deliberately minimal change does not modify. Flash verification is recorded in stable_gui_sense_flash.log. SWD programming settings remain AP1, Under Reset, Hardware reset, 1000 kHz, with verification.

The broader GUI work remains on codex/task-focused-gui. Previously uncommitted local edits were preserved in a named Git stash before switching branches.

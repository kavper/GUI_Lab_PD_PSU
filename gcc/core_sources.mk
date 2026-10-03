# Appli Core C files that the target link must include.
# TouchGFX Designer does not regenerate this fragment.
# psu_sim.c is simulator-only (SIMULATOR / PSU_SIMULATOR) and is not listed.

core_c_files := \
	Appli/Core/Src/main.c \
	Appli/Core/Src/ldo_protocol.c \
	Appli/Core/Src/g0_frame.c \
	Appli/Core/Src/g4_ascii.c \
	Appli/Core/Src/g4_uart.c \
	Appli/Core/Src/psu_limits.c \
	Appli/Core/Src/psu_format.c \
	Appli/Core/Src/psu_edit.c \
	Appli/Core/Src/psu_store.c \
	Appli/Core/Src/psu_seq.c \
	Appli/Core/Src/psu_charger.c \
	Appli/Core/Src/psu_app.c \
	Appli/Core/Src/psu_render.c \
	Appli/Core/Src/ethernetif.c \
	Appli/Core/Src/freertos.c \
	Appli/Core/Src/stm32h7rsxx_it.c \
	Appli/Core/Src/stm32h7rsxx_hal_msp.c \
	Appli/Core/Src/stm32h7rsxx_hal_timebase_tim.c \
	Appli/Core/Src/system_stm32h7rsxx.c

core_c_missing := $(filter-out $(board_c_files),$(core_c_files))
board_c_files += $(core_c_missing)

source_files := $(filter-out Boot/Core/Src/main.c Boot/Core/Src/stm32h7rsxx_hal_msp.c Boot/Core/Src/extmem_manager.c,$(source_files))
source_files += Board/boot_main.c Board/boot_msp.c Board/board_memory.c
include_paths += Board

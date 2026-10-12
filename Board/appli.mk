source_files := $(filter-out Appli/Core/Src/main.c Appli/TouchGFX/target/TouchGFXGPIO.cpp Appli/TouchGFX/target/STM32TouchController.cpp,$(source_files))
c_source_files := $(filter-out Appli/Core/Src/main.c,$(c_source_files))
source_files += Board/appli_main.c Board/board_backlight.c Board/touchgfx_gpio.cpp
include_paths += Board
source_files += Board/touch_controller.cpp

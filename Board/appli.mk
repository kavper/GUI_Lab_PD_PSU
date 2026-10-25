source_files := $(filter-out Appli/Core/Src/main.c Appli/TouchGFX/target/TouchGFXGPIO.cpp Appli/TouchGFX/target/STM32TouchController.cpp,$(source_files))
c_source_files := $(filter-out Appli/Core/Src/main.c,$(c_source_files))
source_files += Board/appli_main.c Board/board_backlight.c Board/board_panel.c Board/touchgfx_gpio.cpp Appli/Core/Src/panel_input.c
include_paths += Board
source_files += Board/touch_controller.cpp
source_files += Board/board_settings.c Appli/Core/Src/psu_nvstore.c

# Board-owned Ethernet/lwIP port; keep generated CubeMX placeholders excluded.
c_source_files := $(filter-out Appli/Core/Src/ethernetif.c,$(c_source_files))
source_files += Board/board_net.c Board/board_http.c
c_source_files += $(wildcard Middlewares/Third_Party/LwIP/src/core/*.c) \
 $(wildcard Middlewares/Third_Party/LwIP/src/core/ipv4/*.c) \
 Middlewares/Third_Party/LwIP/src/netif/ethernet.c
include_paths := Board Middlewares/Third_Party/LwIP/src/include Middlewares/Third_Party/LwIP/system $(include_paths)

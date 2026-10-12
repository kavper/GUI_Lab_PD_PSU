# Build from repository root: make -f Board/loader/Makefile
CC := arm-none-eabi-gcc
MW := Middlewares/ST/STM32_ExtMem_Manager
LOADER := Middlewares/ST/STM32_ExtMem_Loader
OUT := Board/loader/build
HAL := Drivers/STM32H7RSxx_HAL_Driver
SRCS := Board/boot_main.c Board/boot_msp.c Board/board_memory.c Board/loader/api.c \
 Boot/Core/Src/system_stm32h7rsxx.c \
 $(MW)/stm32_extmem.c Board/sfdp_issi.c $(MW)/nor_sfdp/stm32_sfdp_driver.c \
 $(MW)/psram/stm32_psram_driver.c $(MW)/sal/stm32_sal_xspi.c \
 $(LOADER)/STM32Cube/stm32_device_info.c $(LOADER)/core/memory_wrapper.c $(LOADER)/core/systick_management.c \
 $(filter-out %template.c,$(wildcard $(HAL)/Src/*.c))
OBJS := $(patsubst %.c,$(OUT)/%.o,$(SRCS))
INC := Board/loader Board Boot/Core/Inc Drivers/CMSIS/Include Drivers/CMSIS/Device/ST/STM32H7RSxx/Include $(HAL)/Inc $(MW) $(MW)/boot $(LOADER)/STM32Cube $(LOADER)/core
CFLAGS := -Os -g -mcpu=cortex-m7 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -std=gnu11 -ffunction-sections -fdata-sections -MMD -MP -Wall -DUSE_HAL_DRIVER -DSTM32H7S7xx -DBOARD_EXTERNAL_LOADER -DSTM32_EXTMEMLOADER_STM32CUBETARGET $(addprefix -I,$(INC))
.PHONY: all
all: gcc/IS25LX256_GUI_Lab_PD_PSU.stldr
gcc/IS25LX256_GUI_Lab_PD_PSU.stldr: $(OBJS) Board/loader/loader.ld
	$(CC) $(CFLAGS) --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections -Wl,-u,Init,-u,Write,-u,SectorErase,-u,MassErase,-u,Verify,-u,StorageInfo -T Board/loader/loader.ld -Wl,-Map=$(OUT)/loader.map $(OBJS) -o $@
$(OUT)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
-include $(OBJS:.o=.d)

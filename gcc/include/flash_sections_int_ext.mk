_extflash_: _board_preflight_
ifeq ($(program_files_path),)
	$(error $(error_msg))
endif
	# Separate sessions avoid CubeProgrammer's read failure at the second HEX segment.
	@$(objcopy) -O ihex --remove-section=ExtFlashSection --remove-section=FontFlashSection --remove-section=TextFlashSection "$(application_path)/$(binary_output_path)/target.elf" "$(application_path)/$(binary_output_path)/target_code.hex"
	@$(objcopy) -O ihex --only-section=ExtFlashSection --only-section=FontFlashSection --only-section=TextFlashSection "$(application_path)/$(binary_output_path)/target.elf" "$(application_path)/$(binary_output_path)/target_assets.hex"
	@cd "$(st_stm32cube_programmer_path)" && ./$(stm32cube_programmer_filename) -c port=SWD ap=1 mode=UR reset=HWrst freq=1000 -el $(stm32cube_loader_relative_path) -d $(application_path)/$(binary_output_path)/target_code.hex -v
	@cd "$(st_stm32cube_programmer_path)" && ./$(stm32cube_programmer_filename) -c port=SWD ap=1 mode=UR reset=HWrst freq=1000 -el $(stm32cube_loader_relative_path) -d $(application_path)/$(binary_output_path)/target_assets.hex -v -hardRst
_intflash_: _board_preflight_
ifeq ($(program_files_path),)
	$(error $(error_msg))
endif
	@cd "$(st_stm32cube_programmer_path)" && ./$(stm32cube_programmer_filename) -c port=SWD ap=1 mode=UR reset=HWrst freq=1000 -el $(stm32cube_loader_relative_path) -d $(application_path)/$(binary_output_path)/intflash.hex -v -hardRst

.PHONY: _board_preflight_
_board_preflight_:
	@powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(application_path)/Board/preflight.ps1"

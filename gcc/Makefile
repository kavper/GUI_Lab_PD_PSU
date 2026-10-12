makefile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
makefile_dir := $(dir $(makefile_path))


.PHONY: all clean assets flash intflash board_loader

all flash intflash: board_loader
board_loader:
	$(MAKE) -C $(makefile_dir)/.. --no-print-directory -f Board/loader/Makefile

all clean  assets flash intflash:
	$(MAKE) -C $(makefile_dir) --no-print-directory -f makefile_boot $(MAKEFLAGS) $@
	$(MAKE) -C $(makefile_dir) --no-print-directory -f makefile_appli $(MAKEFLAGS) $@
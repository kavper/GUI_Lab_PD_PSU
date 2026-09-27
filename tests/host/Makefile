CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -DPSU_SIMULATOR
INCLUDES := -I../../Appli/Core/Inc

SRC := \
  ../../Appli/Core/Src/g0_frame.c \
  ../../Appli/Core/Src/g4_ascii.c \
  ../../Appli/Core/Src/psu_limits.c \
  ../../Appli/Core/Src/psu_format.c \
  ../../Appli/Core/Src/psu_edit.c \
  ../../Appli/Core/Src/psu_store.c \
  ../../Appli/Core/Src/psu_seq.c \
  ../../Appli/Core/Src/psu_charger.c \
  ../../Appli/Core/Src/psu_app.c \
  ../../Appli/Core/Src/psu_render.c \
  ../../Appli/Core/Src/psu_sim.c \
  test_psu.c

.PHONY: test
test: test_psu
	./test_psu

test_psu: $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRC)

clean:
	rm -f test_psu

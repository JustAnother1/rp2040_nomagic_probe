# automatically created Makefile
# created at: 2025-03-17 01:14:53
# created from mbsp.json

CC = arm-none-eabi-gcc
ASFLAGS = -g -Wall -gdwarf-2 -mthumb -mcpu=cortex-m0plus
SIZE = size
project = mbsp
AS = arm-none-eabi-as
LKR_SCRIPT = hal/RP2040.ld
C_SRC = hal/startup.c hal/irq.c main.c
LFLAGS = -nostartfiles -Wl,-Map=app.map -g -T$(LKR_SCRIPT)
CFLAGS = -c -fno-common -ffreestanding -O3 -g -mcpu=cortex-m0plus -mthumb -I.
SOURCE_DIR = $(dir $(lastword $(MAKEFILE_LIST)))
OBJS =  hal/startup.o hal/irq.o main.o
LD = arm-none-eabi-gcc
CPFLAGS = -Obinary
VPATH = $(SOURCE_DIR)
FILES_TO_CLEAN = app.map
.DEFAULT_GOAL = all

$(project).elf: $(OBJS)
	$(LD) $(LFLAGS) -o $(project).elf $(OBJS)

flash: $(project).elf
	openocd  -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "program $(project).elf verify reset exit"

$(project).uf2: $(project).elf
	elf2uf2 -f 0xe48bff56 -p 256 -i $(project).elf

all: $(project).uf2
	@echo ""
	@echo "size"
	@echo "===="
	$(SIZE) $(project).elf

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(project).elf $(project).uf2 *.o

.PHONY: clean flash all


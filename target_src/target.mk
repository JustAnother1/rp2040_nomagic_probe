# this makefile part creates the files necessary to execute compiled code on the target processor.
# the necessary files are:
# target_progs.h
# target_progs.c

TARGET_SOURCE_FOLDER = target_src/
ELF2BIN = arm-none-eabi-objcopy  -O binary -S

PROGS = blink.c
TARGET_CC = $(CC)
TARGET_CFLAGS = -c -mthumb -nostartfiles -nodefaultlibs -nostdlib -ffreestanding -mcpu=cortex-m0plus -Os -fPIC
TARGET_DDEFS = 
TARGET_INCDIR =

CLEAN_RM += target_src/*.bin target_src/*.d target_src/*.elf target_src/target_progs.c target_src/target_progs.h

BIN_PROGS = $(patsubst %,$(TARGET_SOURCE_FOLDER)%, $(PROGS:.c=.bin))

.PRECIOUS: $(TARGET_SOURCE_FOLDER)%.elf

$(TARGET_SOURCE_FOLDER)%.elf : $(TARGET_SOURCE_FOLDER)%.c
	@echo ""
	@echo "=== compiling $@"
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_DDEFS) $(TARGET_INCDIR) $< -o $@


$(TARGET_SOURCE_FOLDER)%.bin : $(TARGET_SOURCE_FOLDER)%.elf
	@echo ""
	@echo "target elf-> target bin"
	@echo "======================="
	$(ELF2BIN) $< $@


$(TARGET_SOURCE_FOLDER)target_progs.c: $(BIN_PROGS)
	@echo ""
	@echo "create target_progs.c + target_progs.h"
	@echo "======================================"
	@python3 ./target_src/create_api.py $(BIN_PROGS)

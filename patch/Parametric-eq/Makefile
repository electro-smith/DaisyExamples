# Project Name
TARGET = parametric_eq

# Use LGPL version of DaisySP (optional, set to 1 if needed)
USE_DAISYSP_LGPL=1

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

# Source files
# CPP_SOURCES = bytebeat.cpp
CPP_SOURCES = main.cpp

# Include the Daisy system Makefile
include $(SYSTEM_FILES_DIR)/Makefile

# Define the compiled program binary
PROGRAM = parametric_eq.bin
FLASH_ADDR = 0x08000000

clean:
	rm -rf build

# Convert ELF to BIN
build/$(PROGRAM): build/$(TARGET).elf
	arm-none-eabi-objcopy -O binary $< $@

# Flash to Daisy Patch using ST-LINK V3 Mini
program: build/$(PROGRAM)
	st-flash --reset write build/$(PROGRAM) $(FLASH_ADDR)







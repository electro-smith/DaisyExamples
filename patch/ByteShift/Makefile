# Project Name
TARGET = byteshift

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP

# Core location and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

# Source files
CPP_SOURCES = main.cpp byteshift.cpp

# Include the Daisy system Makefile
include $(SYSTEM_FILES_DIR)/Makefile

# Define the compiled program binary
PROGRAM = byteshift.bin
FLASH_ADDR = 0x08000000

# Convert ELF to BIN
build/$(PROGRAM): build/$(TARGET).elf
	arm-none-eabi-objcopy -O binary build/$(TARGET).elf build/$(PROGRAM)

# Flash to Daisy Patch using ST-LINK V3 Mini
program: build/$(PROGRAM)
	st-flash --reset write build/$(PROGRAM) $(FLASH_ADDR)






# # Project Name
# TARGET = byteshift

# # Use LGPL version of DaisySP (optional, set to 1 if needed)
# USE_DAISYSP_LGPL=1

# # Library Locations
# LIBDAISY_DIR = ../../libDaisy
# DAISYSP_DIR = ../../DaisySP

# # Core location and generic Makefile.
# SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

# # Source files
# CPP_SOURCES = main.cpp byteshift.cpp bytebeat_synth.cpp

# # Include the Daisy system Makefile
# include $(SYSTEM_FILES_DIR)/Makefile

# # Ensure .bin file is created from .elf
# build/byteshift.bin: build/byteshift.elf
# 	arm-none-eabi-objcopy -O binary build/byteshift.elf build/byteshift.bin

# # Flashing using STLINK-V3MINI
# PROGRAM = byteshift.bin  # Ensure we're flashing the .bin, not .elf
# FLASH_ADDR = 0x08000000

# program: build/byteshift.bin  # Make sure .bin exists before flashing
# 	st-flash --reset write build/byteshift.bin $(FLASH_ADDR)
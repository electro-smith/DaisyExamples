# Project Name
TARGET = fir

# Library Locations
LIBDAISY_DIR ?= ../../libdaisy
DAISYSP_DIR ?= ../../DaisySP
CMSIS_DIR ?= $(LIBDAISY_DIR)/Drivers/CMSIS


# Sources
CPP_SOURCES = ex_fir.cpp

#C_DEFS += -DUSE_ARM_DSP
#C_SOURCES = $(CMSIS_DIR)/DSP/Source/FilteringFunctions/arm_fir_f32.c   \
#			$(CMSIS_DIR)/DSP/Source/FilteringFunctions/arm_fir_init_f32.c  

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile


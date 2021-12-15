# Project Name
TARGET = Logger

# Sources
CPP_SOURCES = Logger.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP

# Linker flags
# This is not really required, used only for profiling! Increases executable size by ~8kB
LDFLAGS = -u _printf_float


# Core location, and generic makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile


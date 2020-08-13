# Project Name
TARGET = GeneralFunctionTest

# Sources
CPP_SOURCES = GeneralFunctionTest.cpp

# Library Locations
LIBDAISY_DIR = ../../libdaisy
DAISYSP_DIR = ../../DaisySP

# FatFs
FATFS_DIR = $(LIBDAISY_DIR)/Middlewares/Third_Party/FatFs/src
C_SOURCES = \
	$(FATFS_DIR)/diskio.c \
	$(FATFS_DIR)/ff.c \
	$(FATFS_DIR)/ff_gen_drv.c \
	$(FATFS_DIR)/option/ccsbcs.c \

# FATFS dir contains fatfs itself, libdaisy sys folder contains ffconf 
C_INCLUDES = -I$(FATFS_DIR) -I$(LIBDAISY_DIR)/src/sys

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile


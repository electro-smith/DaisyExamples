
ASM_SOURCES = \
$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.s

C_SOURCES = \
$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Source/MatrixFunctions/arm_mat_mult_f32.c \
$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Source/MatrixFunctions/arm_mat_init_f32.c

C_INCLUDES += \
-I$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Include

# Project Name
TARGET = NNWaveshaperB

# Sources
CPP_SOURCES = NNWaveshaperB.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

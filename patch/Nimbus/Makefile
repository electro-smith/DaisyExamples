# Project Name
TARGET = Nimbus

# Sources
CPP_SOURCES += \
Nimbus.cpp \
dsp/correlator.cpp \
dsp/granular_processor.cpp \
dsp/pvoc/frame_transformation.cpp \
dsp/pvoc/phase_vocoder.cpp \
dsp/pvoc/stft.cpp \
resources.cpp \

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP

#C_SOURCES += \
#$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_f32.c \
#$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_rfft_fast_init_f32.c \
#$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Source/CommonTables/arm_const_structs.c \
#$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Source/CommonTables/arm_common_tables.c

#C_DEFS += \
#-DUSE_ARM_FFT \

C_INCLUDES += \
-I. \
-Idsp \
-Idsp/pvoc \
-Idsp/fx \
-Ibootloader \
-Iresources \

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

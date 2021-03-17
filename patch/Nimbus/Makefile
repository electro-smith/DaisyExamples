# Project Name
TARGET = Nimbus

# Sources
CPP_SOURCES += \
Nimbus.cpp \
clouds.cpp \
cv_scaler.cpp \
dsp/correlator.cpp \
dsp/granular_processor.cpp \
dsp/mu_law.cpp \
dsp/pvoc/frame_transformation.cpp \
dsp/pvoc/phase_vocoder.cpp \
dsp/pvoc/stft.cpp \
resources.cpp \
settings.cpp \
atan.cpp \
units.cpp \
system/bootloader_utils.cpp \
system/system_clock.cpp \
utils/random.cpp \
ui.cpp

# Library Locations
LIBDAISY_DIR = ../../libdaisy
DAISYSP_DIR = ../../DaisySP

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

# Project Name
TARGET = Nimbus

# Sources
CPP_SOURCES = \
Nimbus.cpp \
bootloader.cc \
clouds.cc \
cv_scaler.cc \
adc.cc \
codec.cc \
debug_port.cc \
gate_input.cc \
leds.cc \
switches.cc \
system.cc \
correlator.cc \
granular_processor.cc \
mu_law.cc \
frame_transformation.cc \
phase_vocoder.cc \
stft.cc \
resources.cc \
settings.cc \
atan.cc \
units.cc \
system/bootloader_utils.cc \
system/system_clock.cc \
utils/random.cc \
ui.cc

# Library Locations
LIBDAISY_DIR = ../../libdaisy
DAISYSP_DIR = ../../DaisySP

C_INCLUDES += -Ifx -Idsp -Ipvoc -Ibootloader -Idrivers -Iresources -Istmlib

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

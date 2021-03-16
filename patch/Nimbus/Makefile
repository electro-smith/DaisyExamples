# Project Name
TARGET = Nimbus

# Sources
CPP_SOURCES += \
Nimbus.cpp \

CXX_SOURCES += \
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

C_DEFS += -DSTM32F40_41xxx \

C_INCLUDES += \
-I. \
-Idsp \
-Idsp/pvoc \
-Idsp/fx \
-Ibootloader \
-Idrivers \
-Iresources \
-Istmlib \
-Istmlib/algorithms \
-Istmlib/dsp \
-Istmlib/fft \
-Istmlib/linker_scripts \
-Istmlib/midi \
-Istmlib/programming \
-Istmlib/system \
-Istmlib/test \
-Istmlib/third_party/STM \
-Istmlib/third_party/STM/CMSIS/CM3_f4xx \
-Istmlib/third_party/STM/STM32F4xx_StdPeriph_Driver/inc \
-Istmlib/tools \
-Istmlib/ui \
-Istmlib/utils \


# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

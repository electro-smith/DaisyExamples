# Project Name
TARGET = Nimbus

# Sources
CPP_SOURCES += \
Nimbus.cpp \
dsp/correlator.cpp \
dsp/granular_processor.cpp \
dsp/mu_law.cpp \
dsp/pvoc/frame_transformation.cpp \
dsp/pvoc/phase_vocoder.cpp \
dsp/pvoc/stft.cpp \
resources.cpp \
settings.cpp \
stmtemp.cpp \

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

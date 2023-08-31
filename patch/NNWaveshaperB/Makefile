
# for arm_math.h
ASM_SOURCES = \
$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Source/TransformFunctions/arm_bitreversal2.s

# for arm_math.h
C_SOURCES = $(wildcard ../../libDaisy/Drivers/CMSIS/DSP/Source/*/*.c)

CFLAGS += -Wall  # Include all warnings
CFLAGS += -Os    #  Optimize for size

# for arm_math.h
C_INCLUDES += \
-I$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Include

# Project Name
TARGET = NNWaveshaperB


# Sources
CPP_SOURCES = NNWaveshaperB.cpp

# Include C source code for required libraries
CSOURCES += $(wildcard edge-impulse-sdk/CMSIS/DSP/Source/TransformFunctions/*.c) \
			$(wildcard edge-impulse-sdk/CMSIS/DSP/Source/CommonTables/*.c) \
			$(wildcard edge-impulse-sdk/CMSIS/DSP/Source/BasicMathFunctions/*.c) \
			$(wildcard edge-impulse-sdk/CMSIS/DSP/Source/ComplexMathFunctions/*.c) \
			$(wildcard edge-impulse-sdk/CMSIS/DSP/Source/FastMathFunctions/*.c) \
			$(wildcard edge-impulse-sdk/CMSIS/DSP/Source/SupportFunctions/*.c) \
			$(wildcard edge-impulse-sdk/CMSIS/DSP/Source/MatrixFunctions/*.c) \
			$(wildcard edge-impulse-sdk/CMSIS/DSP/Source/StatisticsFunctions/*.c)

# Include C++ source code for required libraries
CXXSOURCES += 	$(wildcard tflite-model/*.cpp) \
				$(wildcard edge-impulse-sdk/dsp/kissfft/*.cpp) \
				$(wildcard edge-impulse-sdk/dsp/dct/*.cpp) \
				$(wildcard edge-impulse-sdk/dsp/memory.cpp) \
				$(wildcard edge-impulse-sdk/porting/posix/*.c*) \
				$(wildcard edge-impulse-sdk/porting/mingw32/*.c*)
CCSOURCES +=

# Use TensorFlow Lite for Microcontrollers (TFLM)
CFLAGS += -DTF_LITE_DISABLE_X86_NEON=1
CSOURCES +=	edge-impulse-sdk/tensorflow/lite/c/common.c
CCSOURCES +=	$(wildcard edge-impulse-sdk/tensorflow/lite/kernels/*.cc) \
				$(wildcard edge-impulse-sdk/tensorflow/lite/kernels/internal/*.cc) \
				$(wildcard edge-impulse-sdk/tensorflow/lite/micro/kernels/*.cc) \
				$(wildcard edge-impulse-sdk/tensorflow/lite/micro/*.cc) \
				$(wildcard edge-impulse-sdk/tensorflow/lite/micro/memory_planner/*.cc) \
				$(wildcard edge-impulse-sdk/tensorflow/lite/core/api/*.cc)

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

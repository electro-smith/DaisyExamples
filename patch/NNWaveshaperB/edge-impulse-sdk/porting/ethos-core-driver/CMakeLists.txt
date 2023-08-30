#
# Copyright (c) 2019-2020 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.15.6)

macro(test_and_convert_error_level log_level ethos_level)
    if(${log_level} STREQUAL ${LOG_LEVEL})
        set(ETHOSU_LOG_SEVERITY ${ethos_level})
    endif()
endmacro()

project(ethosu_core_driver VERSION 0.0.1)

#
# Build options
#

option(DRIVER_PMU_AUTOINIT "Enable PMU boot auto-initialization" OFF)

set(CMSIS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmsis" CACHE PATH "Path to CMSIS.")

set(LOG_NAMES emerg alert crit err warning notice info debug)
if(DEFINED LOG_LEVEL)
    test_and_convert_error_level( LOG_LEVEL_ERROR err )
    test_and_convert_error_level( LOG_LEVEL_DEBUG debug )
    test_and_convert_error_level( LOG_LEVEL_TRACE debug )
    test_and_convert_error_level( LOG_LEVEL_INFO info )
    test_and_convert_error_level( LOG_LEVEL_WARN warning )
else()
    set(ETHOSU_LOG_SEVERITY "info" CACHE STRING "Driver log severity level ${LOG_NAMES}")
    set_property(CACHE ETHOSU_LOG_SEVERITY PROPERTY STRINGS ${LOG_NAMES})
endif()

#
# Global settings
#

# Check that ETHOSU_LOG_SEVERITY has one of the supported levels
list(FIND LOG_NAMES ${ETHOSU_LOG_SEVERITY} LOG_SEVERITY)
if (${LOG_SEVERITY} EQUAL -1)
    message(FATAL_ERROR "Unsupported log level ${ETHOSU_LOG_SEVERITY}")
endif()

# Make include directories available for current- and sub projects
include_directories(include src)
include_directories(${CMSIS_PATH}/CMSIS/Core/Include)

#
# Build libraries
#

# Build driver library
add_library(ethosu_core_driver STATIC)
target_include_directories(ethosu_core_driver PUBLIC include)
target_sources(ethosu_core_driver PRIVATE src/ethosu_driver.c src/ethosu_device.c src/ethosu_pmu.c)

# Set the log level for the target
target_compile_definitions(ethosu_core_driver PRIVATE ETHOSU_LOG_SEVERITY=${LOG_SEVERITY})

# Install library and include files
install(TARGETS ethosu_core_driver LIBRARY DESTINATION "lib")
install(FILES include/ethosu_device.h include/ethosu_driver.h include/pmu_ethosu.h
        DESTINATION "include")

#
# Print build status
#

message(STATUS "*******************************************************")
message(STATUS "PROJECT_NAME                           : ${PROJECT_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR                 : ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMSIS_PATH                             : ${CMSIS_PATH}")
message(STATUS "ETHOSU_LOG_SEVERITY                    : ${ETHOSU_LOG_SEVERITY}")
message(STATUS "*******************************************************")

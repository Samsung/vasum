#Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#
#
# @file   CMakeLists.txt
# @author Jan Olszak (j.olszak@samsung.com)
#

CMAKE_MINIMUM_REQUIRED (VERSION 2.6.2)
PROJECT(vasum)


## pkgconfig ###################################################################
INCLUDE(FindPkgConfig)


## default CMAKE_INSTALL_* variables ###########################################
INCLUDE(GNUInstallDirs)


## Compiler flags, depending on the build type #################################
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "DEBUG")
ENDIF(NOT CMAKE_BUILD_TYPE)
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# special case for a GCC < 4.7, assume rest is fine
IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7)
    SET(CXX_11_STD "c++0x")
else()
    SET(CXX_11_STD "c++11")
endif()

SET(CMAKE_C_FLAGS_PROFILING    "-g -O0 -pg")
SET(CMAKE_CXX_FLAGS_PROFILING  "-g -std=${CXX_11_STD} -O0 -pg")
SET(CMAKE_C_FLAGS_DEBUG        "-g -O0 -ggdb")
SET(CMAKE_CXX_FLAGS_DEBUG      "-g -std=${CXX_11_STD} -O0 -ggdb")
SET(CMAKE_C_FLAGS_RELEASE      "-g -O2 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE    "-g -std=${CXX_11_STD} -O2 -DNDEBUG")
SET(CMAKE_C_FLAGS_CCOV         "-g -O2 --coverage")
SET(CMAKE_CXX_FLAGS_CCOV       "-g -std=${CXX_11_STD} -O2 --coverage")

IF(DEFINED SANITIZE)
    # Enable sanitize build.
    # It works with clang and gcc>=4.8
    # Possible arguments: address, thread and others (see doc.)
    # Note on thread sanitizer bugs left in out code:
    #  - we use non thread save boost test library
    #  - there are some mysterious problems with glib
    SET(SANITIZE_FLAGS "-fsanitize=${SANITIZE}")
    MESSAGE(STATUS "Sanitize flags: ${SANITIZE_FLAGS}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_FLAGS} -fPIE -fno-omit-frame-pointer")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZE_FLAGS} -fPIE -fno-omit-frame-pointer")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie ${SANITIZE_FLAGS}")
ENDIF(DEFINED SANITIZE)

ADD_DEFINITIONS("-fPIC")   # Position Independent Code
ADD_DEFINITIONS("-Werror") # Make all warnings into errors
ADD_DEFINITIONS("-Wall")   # Generate all warnings
ADD_DEFINITIONS("-Wextra") # Generate even more extra warnings
ADD_DEFINITIONS("-pedantic") # Be pedantic
ADD_DEFINITIONS("-pedantic-errors") # Make pedantic warnings into errors
ADD_DEFINITIONS(-DPROGRAM_VERSION="${VERSION}")
ADD_DEFINITIONS(-DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # Warn about documentation problems
    ADD_DEFINITIONS("-Wdocumentation")
    ADD_DEFINITIONS("-Wno-error=documentation")

    IF(ALL_WARNINGS)
        # turn on every -W flags except a few explicitly mentioned
        ADD_DEFINITIONS("-Wno-error")
        ADD_DEFINITIONS("-Weverything")
        ADD_DEFINITIONS("-Wno-c++98-compat")
        ADD_DEFINITIONS("-Wno-c++98-compat-pedantic")
        ADD_DEFINITIONS("-Wno-padded")
        ADD_DEFINITIONS("-Wno-global-constructors")
        ADD_DEFINITIONS("-Wno-exit-time-destructors")
        ADD_DEFINITIONS("-Wno-weak-vtables")
    ENDIF(ALL_WARNINGS)
ENDIF()

IF(NOT DEFINED VASUM_USER)
    SET(VASUM_USER "security-containers")
ENDIF(NOT DEFINED VASUM_USER)
IF(NOT DEFINED INPUT_EVENT_GROUP)
    SET(INPUT_EVENT_GROUP "input")
ENDIF(NOT DEFINED INPUT_EVENT_GROUP)
IF(NOT DEFINED DISK_GROUP)
    SET(DISK_GROUP "disk")
ENDIF(NOT DEFINED DISK_GROUP)
IF(NOT DEFINED TTY_GROUP)
    SET(TTY_GROUP "tty")
ENDIF(NOT DEFINED TTY_GROUP)

ADD_DEFINITIONS(-DVASUM_USER="${VASUM_USER}")
ADD_DEFINITIONS(-DINPUT_EVENT_GROUP="${INPUT_EVENT_GROUP}")
ADD_DEFINITIONS(-DDISK_GROUP="${DISK_GROUP}")
ADD_DEFINITIONS(-DTTY_GROUP="${TTY_GROUP}")

## Python packages directory ###################################################

IF(NOT DEFINED PYTHON_SITELIB)
    EXECUTE_PROCESS(COMMAND python -c
    "from distutils.sysconfig import get_python_lib; import sys; sys.stdout.write(get_python_lib())"
    OUTPUT_VARIABLE PYTHON_SITELIB)
ENDIF(NOT DEFINED PYTHON_SITELIB)


## Subdirectories ##############################################################
SET(COMMON_FOLDER ${PROJECT_SOURCE_DIR}/common)
SET(CLIENT_FOLDER ${PROJECT_SOURCE_DIR}/client)
SET(SERVER_FOLDER ${PROJECT_SOURCE_DIR}/server)
SET(ZONE_SUPPORT_FOLDER ${PROJECT_SOURCE_DIR}/zone-support)
SET(ZONE_DAEMON_FOLDER ${PROJECT_SOURCE_DIR}/zone-daemon)
SET(TESTS_FOLDER ${PROJECT_SOURCE_DIR}/tests)
SET(UNIT_TESTS_FOLDER ${TESTS_FOLDER}/unit_tests)
SET(CLI_FOLDER ${PROJECT_SOURCE_DIR}/cli)

IF(NOT DEFINED SYSCONF_INSTALL_DIR)
    SET(SYSCONF_INSTALL_DIR "/etc")
ENDIF(NOT DEFINED SYSCONF_INSTALL_DIR)

IF(NOT DEFINED LIB_INSTALL_DIR)
    SET(LIB_INSTALL_DIR "${CMAKE_INSTALL_FULL_LIBDIR}")
ENDIF(NOT DEFINED LIB_INSTALL_DIR)

IF(NOT DEFINED INCLUDE_INSTALL_DIR)
    SET(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
ENDIF(NOT DEFINED INCLUDE_INSTALL_DIR)

IF(NOT DEFINED SCRIPT_INSTALL_DIR)
    SET(SCRIPT_INSTALL_DIR "${CMAKE_INSTALL_FULL_SBINDIR}")
ENDIF(NOT DEFINED SCRIPT_INSTALL_DIR)

IF(NOT DEFINED SHARE_INSTALL_PREFIX)
    SET(SHARE_INSTALL_PREFIX "${CMAKE_INSTALL_FULL_DATAROOTDIR}")
ENDIF(NOT DEFINED SHARE_INSTALL_PREFIX)

IF(NOT DEFINED SYSTEMD_UNIT_DIR)
    SET(SYSTEMD_UNIT_DIR "${LIB_INSTALL_DIR}/systemd/system")
ENDIF(NOT DEFINED SYSTEMD_UNIT_DIR)

SET(VSM_CONFIG_INSTALL_DIR ${SYSCONF_INSTALL_DIR}/vasum)
SET(VSM_DATA_INSTALL_DIR ${SHARE_INSTALL_PREFIX}/vasum)

ADD_SUBDIRECTORY(${CLIENT_FOLDER})
ADD_SUBDIRECTORY(${SERVER_FOLDER})
ADD_SUBDIRECTORY(${ZONE_SUPPORT_FOLDER})
ADD_SUBDIRECTORY(${ZONE_DAEMON_FOLDER})
ADD_SUBDIRECTORY(${TESTS_FOLDER})
ADD_SUBDIRECTORY(${CLI_FOLDER})


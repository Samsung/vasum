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
PROJECT(security-containers)


## pkgconfig ###################################################################
INCLUDE(FindPkgConfig)


## default CMAKE_INSTALL_* variables ###########################################
INCLUDE(GNUInstallDirs)


## Compiler flags, depending on the build type #################################
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "DEBUG")
ENDIF(NOT CMAKE_BUILD_TYPE)
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

SET(CMAKE_C_FLAGS_PROFILING    "-g -O0 -pg")
SET(CMAKE_CXX_FLAGS_PROFILING  "-g -std=c++11 -O0 -pg")
SET(CMAKE_C_FLAGS_DEBUG        "-g -O0 -ggdb")
SET(CMAKE_CXX_FLAGS_DEBUG      "-g -std=c++11 -O0 -ggdb")
SET(CMAKE_C_FLAGS_RELEASE      "-g -O2 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE    "-g -std=c++11 -O2 -DNDEBUG")
SET(CMAKE_C_FLAGS_CCOV         "-g -O2 --coverage")
SET(CMAKE_CXX_FLAGS_CCOV       "-g -std=c++11 -O2 --coverage")

ADD_DEFINITIONS("-fPIC")   # Position Independent Code
ADD_DEFINITIONS("-Werror") # Make all warnings into errors
ADD_DEFINITIONS("-Wall")   # Generate all warnings
ADD_DEFINITIONS("-Wextra") # Generate even more extra warnings
ADD_DEFINITIONS("-pedantic") # Be pedantic
ADD_DEFINITIONS("-pedantic-errors") # Make pedantic warnings into errors
ADD_DEFINITIONS(-DPROGRAM_VERSION="${VERSION}")
ADD_DEFINITIONS(-DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")


## Subdirectories ##############################################################
SET(COMMON_FOLDER ${PROJECT_SOURCE_DIR}/common)
SET(CLIENT_FOLDER ${PROJECT_SOURCE_DIR}/client)
SET(SERVER_FOLDER ${PROJECT_SOURCE_DIR}/server)
SET(CONTAINER_DAEMON_FOLDER ${PROJECT_SOURCE_DIR}/container-daemon)
SET(TESTS_FOLDER ${PROJECT_SOURCE_DIR}/tests)
SET(UNIT_TESTS_FOLDER ${TESTS_FOLDER}/unit_tests)

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

SET(SC_CONFIG_INSTALL_DIR ${SYSCONF_INSTALL_DIR}/security-containers)
SET(SC_DATA_INSTALL_DIR ${SHARE_INSTALL_PREFIX}/security-containers)

ADD_SUBDIRECTORY(${CLIENT_FOLDER})
ADD_SUBDIRECTORY(${SERVER_FOLDER})
ADD_SUBDIRECTORY(${CONTAINER_DAEMON_FOLDER})
ADD_SUBDIRECTORY(${TESTS_FOLDER})


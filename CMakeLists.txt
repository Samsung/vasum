# Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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

## File names ##################################################################
SET(SERVER_CODENAME "${PROJECT_NAME}-server")
SET(CLIENT_CODENAME "${PROJECT_NAME}-client")


## Generate the pc file ########################################################
CONFIGURE_FILE(security-containers.pc.in security-containers.pc @ONLY)


## Compiler flags, depending on the build type #################################
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


## Subdirectories ##############################################################
SET(CLIENT_FOLDER ${PROJECT_SOURCE_DIR}/src/client)
SET(SERVER_FOLDER ${PROJECT_SOURCE_DIR}/src/server)

IF(NOT DEFINED SCRIPT_INSTALL_DIR)
    SET(SCRIPT_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/sbin)
ENDIF(NOT DEFINED SCRIPT_INSTALL_DIR)

ADD_SUBDIRECTORY(src)


## Installations ###############################################################
# FILE(MAKE_DIRECTORY /usr/include/security-containers)
INSTALL(FILES       ${CLIENT_FOLDER}/include/security-containers-client.h
        DESTINATION /usr/include/security-containers)

INSTALL(FILES       ${CMAKE_BINARY_DIR}/security-containers.pc
        DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)

INSTALL(FILES       ${SERVER_FOLDER}/config/daemon.conf
        DESTINATION /etc/security-containers/config/)

INSTALL(FILES       ${SERVER_FOLDER}/config/containers/xminimal.conf
        DESTINATION /etc/security-containers/config/containers)

INSTALL(FILES       ${SERVER_FOLDER}/config/libvirt-config/xminimal.xml
        DESTINATION /etc/security-containers/config/libvirt-config)

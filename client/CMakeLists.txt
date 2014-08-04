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
# @author Mateusz Malicki (m.malicki2@samsung.com)
#

MESSAGE(STATUS "Generating makefile for the Client...")
FILE(GLOB project_SRCS *.cpp *.hpp *.h)
FILE(GLOB common_SRCS ${COMMON_FOLDER}/utils/callback-guard.hpp
                      ${COMMON_FOLDER}/utils/callback-guard.cpp
                      ${COMMON_FOLDER}/utils/glib-loop.cpp
                      ${COMMON_FOLDER}/utils/glib-loop.hpp)

SET(_LIB_VERSION_ "0.0.1")
SET(_LIB_SOVERSION_ "0")
SET(PC_FILE "${PROJECT_NAME}.pc")

## Set the default ELF image symbol visibility to hidden. Using this feature
## can very substantially improve linking and load times of shared object
## libraries, produce more optimized code, provide near-perfect API export
## and prevent symbol clashes
ADD_DEFINITIONS(-fvisibility=hidden)

## Setup target ################################################################
ADD_LIBRARY(${PROJECT_NAME} SHARED ${project_SRCS} ${common_SRCS})
SET_PROPERTY(TARGET ${PROJECT_NAME} PROPERTY SOVERSION ${_LIB_SOVERSION_})
SET_PROPERTY(TARGET ${PROJECT_NAME} PROPERTY   VERSION ${_LIB_VERSION_})


## Link libraries ##############################################################
PKG_CHECK_MODULES(LIB_DEPS REQUIRED gio-2.0 libSimpleDbus libLogger)
INCLUDE_DIRECTORIES(SYSTEM ${LIB_DEPS_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${COMMON_FOLDER})
INCLUDE_DIRECTORIES(${SERVER_FOLDER})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_DEPS_LIBRARIES})

## Generate the pc file ########################################################
CONFIGURE_FILE(${PC_FILE}.in ${CMAKE_BINARY_DIR}/${PC_FILE} @ONLY)


## Install #####################################################################
INSTALL(FILES       ${CMAKE_BINARY_DIR}/${PC_FILE}
        DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)

INSTALL(TARGETS     ${PROJECT_NAME}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   RuntimeLibraries)

INSTALL(FILES       security-containers-client.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/${PROJECT_NAME})

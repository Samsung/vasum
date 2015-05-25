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

PROJECT(vasum)
MESSAGE(STATUS "")
MESSAGE(STATUS "Generating makefile for the Vasum wrapper...")

## set wrapper library sources (without vasum-client.cpp)
FILE(GLOB wrapper_SRCS *.cpp *.hpp *.h
     ${CLIENT_FOLDER}/vasum-client-impl.hpp     ${CLIENT_FOLDER}/vasum-client-impl.cpp
     ${CLIENT_FOLDER}/host-ipc-connection.hpp   ${CLIENT_FOLDER}/host-ipc-connection.cpp
     ${CLIENT_FOLDER}/utils.hpp                 ${CLIENT_FOLDER}/utils.cpp)

FILE(GLOB common_SRCS ${COMMON_FOLDER}/epoll/*.hpp              ${COMMON_FOLDER}/epoll/*.cpp
                      ${COMMON_FOLDER}/ipc/*.hpp                ${COMMON_FOLDER}/ipc/*.cpp
                      ${COMMON_FOLDER}/ipc/internals/*.hpp      ${COMMON_FOLDER}/ipc/internals/*.cpp
                      ${COMMON_FOLDER}/utils/*.hpp              ${COMMON_FOLDER}/utils/*.cpp
                      ${COMMON_FOLDER}/*.hpp                    ${COMMON_FOLDER}/*.cpp)

SET(_LIB_VERSION_ "${VERSION}")
SET(_LIB_SOVERSION_ "0")
SET(PC_FILE "${PROJECT_NAME}.pc")

## Set the default ELF image symbol visibility to hidden. Using this feature
## can very substantially improve linking and load times of shared object
## libraries, produce more optimized code, provide near-perfect API export
## and prevent symbol clashes
ADD_DEFINITIONS(-fvisibility=hidden)
ADD_DEFINITIONS(-D__STDC_FORMAT_MACROS)

ADD_LIBRARY(${PROJECT_NAME} SHARED ${wrapper_SRCS} ${common_SRCS})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
        SOVERSION ${_LIB_SOVERSION_}
        VERSION ${_LIB_VERSION_}
        COMPILE_DEFINITIONS HOST_IPC_SOCKET="${VSM_SERVER_IPC_SOCKET_PATH}")

## Link libraries ##############################################################
FIND_PACKAGE(Boost COMPONENTS system filesystem)
PKG_CHECK_MODULES(LIB_DEPS REQUIRED gio-2.0)
INCLUDE_DIRECTORIES(SYSTEM ${LIB_DEPS_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${COMMON_FOLDER})
INCLUDE_DIRECTORIES(${LIBS_FOLDER})
INCLUDE_DIRECTORIES(${SERVER_FOLDER})
INCLUDE_DIRECTORIES(${CLIENT_FOLDER})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_DEPS_LIBRARIES} ${Boost_LIBRARIES}
                      Config SimpleDbus Ipc)

## Generate the pc file ########################################################
CONFIGURE_FILE(${PC_FILE}.in ${CMAKE_BINARY_DIR}/${PC_FILE} @ONLY)


## Install #####################################################################
INSTALL(FILES       ${CMAKE_BINARY_DIR}/${PC_FILE}
        DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)

INSTALL(TARGETS     ${PROJECT_NAME}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   RuntimeLibraries)

INSTALL(FILES       vasum.h vasum_list.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/${PROJECT_NAME})

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

MESSAGE(STATUS "")
MESSAGE(STATUS "Generating makefile for the Server...")
FILE(GLOB               project_SRCS *.cpp *.hpp)


## Setup target ################################################################
SET(SERVER_CODENAME "${PROJECT_NAME}-server")
ADD_EXECUTABLE(${SERVER_CODENAME} ${project_SRCS})

ADD_DEPENDENCIES(${SERVER_CODENAME} Common Logger Cargo Ipc)

## Link libraries ##############################################################
FIND_PACKAGE(Boost COMPONENTS program_options system filesystem ${USE_BOOST_REGEX})
PKG_CHECK_MODULES(SERVER_DEPS REQUIRED lxc glib-2.0 gio-2.0)

INCLUDE_DIRECTORIES(${COMMON_FOLDER})
INCLUDE_DIRECTORIES(${LIBS_FOLDER})
INCLUDE_DIRECTORIES(${CLIENT_FOLDER})
INCLUDE_DIRECTORIES(SYSTEM ${SERVER_DEPS_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${JSON_C_INCLUDE_DIRS})

SET_TARGET_PROPERTIES(${SERVER_CODENAME} PROPERTIES
    COMPILE_FLAGS "-pthread"
    COMPILE_DEFINITIONS HOST_IPC_SOCKET="${VSM_SERVER_IPC_SOCKET_PATH}"
    LINK_FLAGS "-pthread"
)

TARGET_LINK_LIBRARIES(${SERVER_CODENAME} Common ${SERVER_DEPS_LIBRARIES} ${Boost_LIBRARIES} Logger Cargo Ipc)
IF(NOT WITHOUT_DBUS)
TARGET_LINK_LIBRARIES(${SERVER_CODENAME} SimpleDbus)
ENDIF(NOT WITHOUT_DBUS)

## Subdirectories ##############################################################
ADD_SUBDIRECTORY(configs)


## Install #####################################################################
INSTALL(TARGETS ${SERVER_CODENAME} DESTINATION bin)

## Set capabilities on server executable #######################################
INSTALL(CODE "EXECUTE_PROCESS(COMMAND setcap CAP_SYS_ADMIN,CAP_MAC_OVERRIDE,CAP_SYS_TTY_CONFIG+ei \$ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}/bin/${SERVER_CODENAME})")

INSTALL(PROGRAMS "scripts/vasum-check-config.sh" DESTINATION sbin RENAME vasum-check-config)

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
MESSAGE(STATUS "Generating makefile for the Zone Daemon...")
FILE(GLOB project_SRCS *.cpp *.hpp)

## Setup target ################################################################
SET(ZONE_DAEMON_CODENAME "${PROJECT_NAME}-zone-daemon")
ADD_EXECUTABLE(${ZONE_DAEMON_CODENAME} ${project_SRCS})

ADD_DEPENDENCIES(${ZONE_DAEMON_CODENAME} Common Logger SimpleDbus Ipc)

## Link libraries ##############################################################
FIND_PACKAGE (Boost COMPONENTS program_options system filesystem)

PKG_CHECK_MODULES(ZONE_DAEMON_DEPS REQUIRED gio-2.0)

INCLUDE_DIRECTORIES(${COMMON_FOLDER})
INCLUDE_DIRECTORIES(${LIBS_FOLDER})
INCLUDE_DIRECTORIES(SYSTEM ${ZONE_DAEMON_DEPS_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

SET_TARGET_PROPERTIES(${ZONE_DAEMON_CODENAME} PROPERTIES
    COMPILE_FLAGS "-pthread"
    LINK_FLAGS "-pthread"
)

TARGET_LINK_LIBRARIES(${ZONE_DAEMON_CODENAME} Common ${ZONE_DAEMON_DEPS_LIBRARIES}
                      ${Boost_LIBRARIES} Logger SimpleDbus Ipc)


## Install #####################################################################
INSTALL(TARGETS ${ZONE_DAEMON_CODENAME} DESTINATION bin)

CONFIGURE_FILE(configs/org.tizen.vasum.zone.daemon.conf.in
               ${CMAKE_BINARY_DIR}/configs/org.tizen.vasum.zone.daemon.conf)

INSTALL(FILES       ${CMAKE_BINARY_DIR}/configs/org.tizen.vasum.zone.daemon.conf
        DESTINATION /etc/dbus-1/system.d/)

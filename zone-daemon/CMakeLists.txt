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
FILE(GLOB common_SRCS ${COMMON_FOLDER}/dbus/*.cpp   ${COMMON_FOLDER}/dbus/*.hpp
                      ${COMMON_FOLDER}/logger/*.cpp ${COMMON_FOLDER}/logger/*.hpp
                      ${COMMON_FOLDER}/utils/*.cpp  ${COMMON_FOLDER}/utils/*.hpp
                      ${COMMON_FOLDER}/*.cpp)

## Setup target ################################################################
SET(ZONE_DAEMON_CODENAME "${PROJECT_NAME}-zone-daemon")
ADD_EXECUTABLE(${ZONE_DAEMON_CODENAME} ${project_SRCS} ${common_SRCS})


## Link libraries ##############################################################
FIND_PACKAGE (Boost COMPONENTS program_options system filesystem)

PKG_CHECK_MODULES(ZONE_DAEMON_DEPS REQUIRED gio-2.0 libsystemd-journal libcap-ng)

INCLUDE_DIRECTORIES(${COMMON_FOLDER})
INCLUDE_DIRECTORIES(SYSTEM ${ZONE_DAEMON_DEPS_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

SET_TARGET_PROPERTIES(${ZONE_DAEMON_CODENAME} PROPERTIES
    COMPILE_FLAGS "-pthread"
    LINK_FLAGS "-pthread"
)

TARGET_LINK_LIBRARIES(${ZONE_DAEMON_CODENAME} ${ZONE_DAEMON_DEPS_LIBRARIES} ${Boost_LIBRARIES})


## Install #####################################################################
INSTALL(TARGETS ${ZONE_DAEMON_CODENAME} DESTINATION bin)

CONFIGURE_FILE(configs/org.tizen.vasum.zone.daemon.conf.in
               ${CMAKE_BINARY_DIR}/configs/org.tizen.vasum.zone.daemon.conf)

INSTALL(FILES       ${CMAKE_BINARY_DIR}/configs/org.tizen.vasum.zone.daemon.conf
        DESTINATION /etc/dbus-1/system.d/)

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

MESSAGE(STATUS "Generating makefile for the Container Daemon...")
FILE(GLOB project_SRCS *.cpp *.hpp)
FILE(GLOB common_SRCS ${COMMON_FOLDER}/dbus/*.cpp  ${COMMON_FOLDER}/dbus/*.hpp
                      ${COMMON_FOLDER}/log/*.cpp   ${COMMON_FOLDER}/log/*.hpp
                      ${COMMON_FOLDER}/utils/*.cpp ${COMMON_FOLDER}/utils/*.hpp)

## Setup target ################################################################
SET(CONTAINER_DAEMON_CODENAME "${PROJECT_NAME}-container-daemon")
ADD_EXECUTABLE(${CONTAINER_DAEMON_CODENAME} ${project_SRCS} ${common_SRCS})


## Link libraries ##############################################################
FIND_PACKAGE (Boost COMPONENTS program_options system filesystem)

PKG_CHECK_MODULES(CONTAINER_DAEMON_DEPS REQUIRED gio-2.0 libsystemd-journal)
INCLUDE_DIRECTORIES(${COMMON_FOLDER})
INCLUDE_DIRECTORIES(SYSTEM ${CONTAINER_DAEMON_DEPS_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${CONTAINER_DAEMON_CODENAME} ${CONTAINER_DAEMON_DEPS_LIBRARIES} ${Boost_LIBRARIES})


## Install #####################################################################
INSTALL(TARGETS ${CONTAINER_DAEMON_CODENAME} DESTINATION bin)

INSTALL(FILES       configs/org.tizen.container.daemon.conf
        DESTINATION /etc/dbus-1/system.d/)

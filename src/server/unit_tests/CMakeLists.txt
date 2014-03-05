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

MESSAGE(STATUS "Generating makefile for the Server unit tests...")
FILE(GLOB_RECURSE project_SRCS *.cpp)
FILE(GLOB src_SRCS ${SERVER_FOLDER}/src/scs-container-admin.cpp
                   ${SERVER_FOLDER}/src/scs-container.cpp
                   ${SERVER_FOLDER}/src/scs-container-manager.cpp
                   ${SERVER_FOLDER}/src/scs-configuration.cpp
                   ${SERVER_FOLDER}/src/dbus-connection.cpp
                   ${SERVER_FOLDER}/src/glib-loop.cpp
                   ${SERVER_FOLDER}/src/file-wait.cpp
                   ${SERVER_FOLDER}/src/latch.cpp)

## Setup target ################################################################
SET(UT_SERVER_CODENAME "${PROJECT_NAME}-server-unit-tests")
ADD_EXECUTABLE(${UT_SERVER_CODENAME} ${project_SRCS} ${src_SRCS})

## Link libraries ##############################################################
FIND_PACKAGE (Boost COMPONENTS unit_test_framework REQUIRED)

PKG_CHECK_MODULES(UT_SERVER_DEPS REQUIRED libvirt json gio-2.0)

INCLUDE_DIRECTORIES(SYSTEM ${UT_SERVER_DEPS_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${UT_SERVER_CODENAME} ${UT_SERVER_DEPS_LIBRARIES} ${Boost_LIBRARIES})

## Install #####################################################################
INSTALL(TARGETS ${UT_SERVER_CODENAME} DESTINATION bin)

FILE(GLOB manager_CONF         ${SERVER_FOLDER}/unit_tests/config/ut-scs-container-manager/*.conf)
FILE(GLOB container_CONF       ${SERVER_FOLDER}/unit_tests/config/ut-scs-container-manager/containers/*.conf)
FILE(GLOB containeradmin_CONF  ${SERVER_FOLDER}/unit_tests/config/ut-scs-container-manager/libvirt-config/*.xml)
FILE(GLOB dbus_CONF            ${SERVER_FOLDER}/unit_tests/config/ut-dbus-connection/*.conf)

INSTALL(FILES        ${manager_CONF}
        DESTINATION  ${SC_CONFIG_INSTALL_DIR}/tests/ut-scs-container-manager)
INSTALL(FILES        ${container_CONF}
        DESTINATION  ${SC_CONFIG_INSTALL_DIR}/tests/ut-scs-container-manager/containers)
INSTALL(FILES        ${containeradmin_CONF}
        DESTINATION  ${SC_CONFIG_INSTALL_DIR}/tests/ut-scs-container-manager/libvirt-config)
INSTALL(FILES        ${dbus_CONF}
        DESTINATION  ${SC_CONFIG_INSTALL_DIR}/tests/ut-dbus-connection)

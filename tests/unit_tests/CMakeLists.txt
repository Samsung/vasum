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

MESSAGE(STATUS "Generating makefile for the Unit Tests...")
FILE(GLOB_RECURSE project_SRCS *.cpp *.hpp)
FILE(GLOB_RECURSE common_SRCS ${COMMON_FOLDER}/*.cpp ${COMMON_FOLDER}/*.hpp)
FILE(GLOB         server_SRCS ${SERVER_FOLDER}/*.cpp ${SERVER_FOLDER}/*.hpp)
FILE(GLOB         client_SRCS ${CLIENT_FOLDER}/*.cpp ${CLIENT_FOLDER}/*.h)

FILE(GLOB         main_SRC ${SERVER_FOLDER}/main.cpp)
LIST(REMOVE_ITEM server_SRCS ${main_SRC})


## Setup target ################################################################
SET(UT_SERVER_CODENAME "${PROJECT_NAME}-server-unit-tests")
ADD_EXECUTABLE(${UT_SERVER_CODENAME} ${project_SRCS} ${common_SRCS} ${server_SRCS} ${client_SRCS})


## Link libraries ##############################################################
FIND_PACKAGE (Boost COMPONENTS unit_test_framework system filesystem regex)

PKG_CHECK_MODULES(UT_SERVER_DEPS REQUIRED libvirt libvirt-glib-1.0 json gio-2.0
                  libsystemd-journal libcap-ng libLogger libSimpleDbus libConfig
                  libsystemd-daemon)
INCLUDE_DIRECTORIES(${COMMON_FOLDER} ${SERVER_FOLDER} ${UNIT_TESTS_FOLDER} ${CLIENT_FOLDER})
INCLUDE_DIRECTORIES(SYSTEM ${UT_SERVER_DEPS_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${UT_SERVER_CODENAME} ${UT_SERVER_DEPS_LIBRARIES} ${Boost_LIBRARIES})


## Subdirectories ##############################################################
SET(SC_TEST_CONFIG_INSTALL_DIR ${SC_DATA_INSTALL_DIR}/tests)
ADD_DEFINITIONS(-DSC_TEST_CONFIG_INSTALL_DIR="${SC_TEST_CONFIG_INSTALL_DIR}")

ADD_SUBDIRECTORY(dbus/configs)
ADD_SUBDIRECTORY(server/configs)
ADD_SUBDIRECTORY(utils/configs)
ADD_SUBDIRECTORY(client/configs)


## Install #####################################################################
INSTALL(TARGETS ${UT_SERVER_CODENAME} DESTINATION bin)

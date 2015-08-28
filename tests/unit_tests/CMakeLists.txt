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
MESSAGE(STATUS "Generating makefile for the Unit Tests...")
FILE(GLOB_RECURSE project_SRCS *.cpp *.hpp)
FILE(GLOB_RECURSE common_SRCS ${COMMON_FOLDER}/*.cpp ${COMMON_FOLDER}/*.hpp)
FILE(GLOB         server_SRCS ${SERVER_FOLDER}/*.cpp ${SERVER_FOLDER}/*.hpp)
FILE(GLOB         client_SRCS ${CLIENT_FOLDER}/*.cpp ${CLIENT_FOLDER}/*.h)
FILE(GLOB         socket_test_SRCS ${SOCKET_TEST_FOLDER}/*.cpp ${SOCKET_TEST_FOLDER}/*.hpp)
FILE(GLOB         lxcpp_test_SRCS lxcpp/lxcpp-api-compile-test.cpp)

FILE(GLOB         main_SRC ${SERVER_FOLDER}/main.cpp)
LIST(REMOVE_ITEM server_SRCS ${main_SRC})

# We must compile socket-test separately, exclude it from unit-test build
LIST(REMOVE_ITEM project_SRCS ${socket_test_SRCS} ${lxcpp_test_SRCS})


## Setup target ################################################################
SET(UT_SERVER_CODENAME "${PROJECT_NAME}-server-unit-tests")
ADD_EXECUTABLE(${UT_SERVER_CODENAME} ${project_SRCS} ${common_SRCS} ${server_SRCS} ${client_SRCS})

## A fake target to test vasum-client C API
ADD_EXECUTABLE("vasum-client-c-api-compile-test" client/client-c-api-compile-test.c)

## A fake target to test lxcpp API
ADD_EXECUTABLE("lxcpp-api-compile-test" lxcpp/lxcpp-api-compile-test.cpp)
TARGET_LINK_LIBRARIES("lxcpp-api-compile-test" lxcpp)

IF(NOT WITHOUT_SYSTEMD)
SET(SOCKET_TEST_CODENAME "${PROJECT_NAME}-socket-test")

## A stub mini-service to test socket functionality
ADD_EXECUTABLE(${SOCKET_TEST_CODENAME} ${socket_test_SRCS} ${common_SRCS} ${client_SRCS})
ENDIF(NOT WITHOUT_SYSTEMD)

## Link libraries ##############################################################
FIND_PACKAGE (Boost COMPONENTS unit_test_framework system filesystem regex)

PKG_CHECK_MODULES(UT_SERVER_DEPS REQUIRED lxc gio-2.0)
INCLUDE_DIRECTORIES(${COMMON_FOLDER} ${SERVER_FOLDER} ${UNIT_TESTS_FOLDER} ${CLIENT_FOLDER}
                    ${LIBS_FOLDER} ${SOCKET_TEST_FOLDER})
INCLUDE_DIRECTORIES(SYSTEM ${UT_SERVER_DEPS_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${JSON_C_INCLUDE_DIRS})

SET_TARGET_PROPERTIES(${UT_SERVER_CODENAME} PROPERTIES
    COMPILE_FLAGS "-pthread"
    COMPILE_DEFINITIONS HOST_IPC_SOCKET="${VSM_UNIT_TESTS_IPC_SOCKET_PATH}"
    LINK_FLAGS "-pthread"
)

TARGET_LINK_LIBRARIES(${UT_SERVER_CODENAME} ${UT_SERVER_DEPS_LIBRARIES} ${Boost_LIBRARIES}
                      Logger Config Ipc lxcpp)
IF(NOT WITHOUT_DBUS)
TARGET_LINK_LIBRARIES(${UT_SERVER_CODENAME} SimpleDbus)
ENDIF(NOT WITHOUT_DBUS)

IF(NOT WITHOUT_SYSTEMD)
SET_TARGET_PROPERTIES(${SOCKET_TEST_CODENAME} PROPERTIES
    COMPILE_FLAGS "-pthread"
    COMPILE_DEFINITIONS HOST_IPC_SOCKET="${VSM_UNIT_TESTS_IPC_SOCKET_PATH}"
    LINK_FLAGS "-pthread"
)

TARGET_LINK_LIBRARIES(${SOCKET_TEST_CODENAME} ${UT_SERVER_DEPS_LIBRARIES} ${Boost_LIBRARIES}
                      Logger Config Ipc)
IF(NOT WITHOUT_DBUS)
TARGET_LINK_LIBRARIES(${SOCKET_TEST_CODENAME} SimpleDbus)
ENDIF(NOT WITHOUT_DBUS)

ENDIF(NOT WITHOUT_SYSTEMD)

## Subdirectories ##############################################################
SET(VSM_TEST_CONFIG_INSTALL_DIR ${VSM_CONFIG_INSTALL_DIR}/tests)
SET(VSM_TEST_TEMPLATES_INSTALL_DIR ${VSM_CONFIG_INSTALL_DIR}/tests/templates)
ADD_DEFINITIONS(-DVSM_TEST_CONFIG_INSTALL_DIR="${VSM_TEST_CONFIG_INSTALL_DIR}")
ADD_DEFINITIONS(-DVSM_TEST_TEMPLATES_INSTALL_DIR="${VSM_TEST_TEMPLATES_INSTALL_DIR}")

ADD_SUBDIRECTORY(configs)

## Install #####################################################################
INSTALL(TARGETS ${UT_SERVER_CODENAME} DESTINATION bin)

IF(NOT WITHOUT_SYSTEMD)
INSTALL(TARGETS ${SOCKET_TEST_CODENAME} DESTINATION bin)
ENDIF(NOT WITHOUT_SYSTEMD)


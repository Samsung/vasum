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
FILE(GLOB         server_SRCS ${SERVER_FOLDER}/*.cpp ${SERVER_FOLDER}/*.hpp
                              ${COMMON_FOLDER}/lxc/*.cpp ${COMMON_FOLDER}/lxc/*.hpp
                              ${COMMON_FOLDER}/api/*.cpp ${COMMON_FOLDER}/api/*.hpp)
FILE(GLOB         client_SRCS ${CLIENT_FOLDER}/*.cpp ${CLIENT_FOLDER}/*.h)
FILE(GLOB         lxcpp_test_SRCS lxcpp/lxcpp-api-compile-test.cpp)
FILE(GLOB         lxcpp_init_SRCS lxcpp/lxcpp-simple-init.c)
FILE(GLOB         lxcpp_ls_SRCS lxcpp/lxcpp-simple-ls.c)
FILE(GLOB         lxcpp_rand_SRCS lxcpp/lxcpp-simple-rand.c)

FILE(GLOB         main_SRC ${SERVER_FOLDER}/main.cpp)
LIST(REMOVE_ITEM server_SRCS ${main_SRC})

# We must compile some tests/tools separately, exclude them from unit-test build
LIST(REMOVE_ITEM project_SRCS ${lxcpp_test_SRCS} ${lxcpp_init_SRCS})

SET(LAUNCHER_CODENAME "${PROJECT_NAME}-launcher")

## Setup target ################################################################
SET(UT_SERVER_CODENAME "${PROJECT_NAME}-server-unit-tests")
ADD_EXECUTABLE(${UT_SERVER_CODENAME} ${project_SRCS} ${server_SRCS} ${client_SRCS})
ADD_DEPENDENCIES(${UT_SERVER_CODENAME} ${PROJECT_NAME}-server)

## A fake target to test vasum-client C API
SET(CLIENT_C_API_COMPILE_TEST "vasum-client-c-api-compile-test")
ADD_EXECUTABLE(${CLIENT_C_API_COMPILE_TEST} client/client-c-api-compile-test.c)
ADD_DEPENDENCIES(${CLIENT_C_API_COMPILE_TEST} ${PROJECT_NAME}-client)

## A fake target to test lxcpp API
SET(LXCPP_API_COMPILE_TEST "lxcpp-api-compile-test")
ADD_EXECUTABLE(${LXCPP_API_COMPILE_TEST} ${lxcpp_test_SRCS})
ADD_DEPENDENCIES(${LXCPP_API_COMPILE_TEST} lxcpp)
TARGET_LINK_LIBRARIES(${LXCPP_API_COMPILE_TEST} lxcpp)

## A small init binary used by container tests
SET(LXCPP_SIMPLE_INIT "lxcpp-simple-init")
ADD_EXECUTABLE(${LXCPP_SIMPLE_INIT} ${lxcpp_init_SRCS})
SET_TARGET_PROPERTIES(${LXCPP_SIMPLE_INIT} PROPERTIES LINK_FLAGS "-static")

## A small list files binary used by container provisioning tests
SET(LXCPP_SIMPLE_LS "lxcpp-simple-ls")
ADD_EXECUTABLE(${LXCPP_SIMPLE_LS} ${lxcpp_ls_SRCS})
SET_TARGET_PROPERTIES(${LXCPP_SIMPLE_LS} PROPERTIES LINK_FLAGS "-static")

## A generate random bytes binary used by container attach tests
SET(LXCPP_SIMPLE_RAND "lxcpp-simple-rand")
ADD_EXECUTABLE(${LXCPP_SIMPLE_RAND} ${lxcpp_rand_SRCS})
SET_TARGET_PROPERTIES(${LXCPP_SIMPLE_RAND} PROPERTIES LINK_FLAGS "-static")

## Link libraries ##############################################################
FIND_PACKAGE (Boost REQUIRED COMPONENTS unit_test_framework iostreams)

INCLUDE_DIRECTORIES(${COMMON_FOLDER} ${LIBS_FOLDER} ${SERVER_FOLDER} ${UNIT_TESTS_FOLDER} ${CLIENT_FOLDER})
INCLUDE_DIRECTORIES(SYSTEM ${CARGO_UTILS_INCLUDE_DIRS} ${DBUS_DEPS_INCLUDE_DIRS} ${SERVER_DEPS_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIRS} ${LXCPP_DEPS_INCLUDE_DIRS})

SET_TARGET_PROPERTIES(${UT_SERVER_CODENAME} PROPERTIES
    COMPILE_FLAGS "-pthread"
    COMPILE_DEFINITIONS HOST_IPC_SOCKET="${VSM_UNIT_TESTS_IPC_SOCKET_PATH}"
    LINK_FLAGS "-pthread"
)
TARGET_COMPILE_DEFINITIONS(${UT_SERVER_CODENAME}
    PRIVATE LAUNCHER_PATH="${LIBEXEC_DIR}/${LAUNCHER_CODENAME}"
    PRIVATE SIMPLE_INIT_PATH="${LIBEXEC_DIR}/${LXCPP_SIMPLE_INIT}"
    PRIVATE SIMPLE_LS_PATH="${LIBEXEC_DIR}/${LXCPP_SIMPLE_LS}"
    PRIVATE SIMPLE_RAND_PATH="${LIBEXEC_DIR}/${LXCPP_SIMPLE_RAND}"
)

TARGET_LINK_LIBRARIES(${UT_SERVER_CODENAME} ${CARGO_UTILS_LIBRARIES} ${SERVER_DEPS_LIBRARIES} ${Boost_LIBRARIES} lxcpp)

IF(NOT WITHOUT_DBUS)
TARGET_LINK_LIBRARIES(${UT_SERVER_CODENAME} SimpleDbus)
ENDIF(NOT WITHOUT_DBUS)

## Subdirectories ##############################################################
SET(VSM_TEST_CONFIG_INSTALL_DIR ${VSM_CONFIG_INSTALL_DIR}/tests)
SET(VSM_TEST_TEMPLATES_INSTALL_DIR ${VSM_CONFIG_INSTALL_DIR}/tests/templates)
ADD_DEFINITIONS(-DVSM_TEST_CONFIG_INSTALL_DIR="${VSM_TEST_CONFIG_INSTALL_DIR}")
ADD_DEFINITIONS(-DVSM_TEST_TEMPLATES_INSTALL_DIR="${VSM_TEST_TEMPLATES_INSTALL_DIR}")

ADD_SUBDIRECTORY(configs)

## Install #####################################################################
INSTALL(TARGETS ${UT_SERVER_CODENAME} DESTINATION bin)
INSTALL(TARGETS ${LXCPP_SIMPLE_INIT} DESTINATION ${LIBEXEC_DIR})
INSTALL(TARGETS ${LXCPP_SIMPLE_LS} DESTINATION ${LIBEXEC_DIR})
INSTALL(TARGETS ${LXCPP_SIMPLE_RAND} DESTINATION ${LIBEXEC_DIR})

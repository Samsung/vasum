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

MESSAGE(STATUS "Generating makefile for the Client...")
FILE(GLOB         project_SRCS *.cpp *.hpp)
FILE(GLOB_RECURSE common_SRCS ${COMMON_FOLDER}/*.cpp ${COMMON_FOLDER}/*.hpp)


## Setup target ################################################################
SET(CLIENT_CODENAME "${PROJECT_NAME}-client")
ADD_LIBRARY(${CLIENT_CODENAME} SHARED ${project_SRCS})


## Link libraries ##############################################################
INCLUDE_DIRECTORIES(${COMMON_FOLDER})


## Generate the pc file ########################################################
CONFIGURE_FILE(security-containers.pc.in
               ${CMAKE_BINARY_DIR}/security-containers.pc @ONLY)


## Install #####################################################################
INSTALL(FILES       ${CMAKE_BINARY_DIR}/security-containers.pc
        DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)

INSTALL(TARGETS     ${CLIENT_CODENAME}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   RuntimeLibraries)

INSTALL(FILES       security-containers-client.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/security-containers)

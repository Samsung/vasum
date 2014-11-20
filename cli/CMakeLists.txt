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

MESSAGE(STATUS "")
MESSAGE(STATUS "Generating makefile for the command line interface...")
FILE(GLOB cli_SRCS *.cpp *.hpp)

## Setup target ################################################################
SET(CLI_CODENAME "${PROJECT_NAME}-cli")
ADD_EXECUTABLE(${CLI_CODENAME} ${cli_SRCS})


## Link libraries ##############################################################
PKG_CHECK_MODULES(LIB_DEPS REQUIRED security-containers)

INCLUDE_DIRECTORIES(${CLIENT_FOLDER})
INCLUDE_DIRECTORIES(${COMMON_FOLDER})
TARGET_LINK_LIBRARIES(${CLI_CODENAME} ${LIB_DEPS_LIBRARIES} ${PROJECT_NAME})


## Install #####################################################################
INSTALL(TARGETS ${CLI_CODENAME} DESTINATION bin)

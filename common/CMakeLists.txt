# Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
# @author Pawel Kubik (p.kubik@samsung.com)
#

MESSAGE(STATUS "")
MESSAGE(STATUS "Generating makefile for the libCommon...")
FILE(GLOB_RECURSE SRCS *.cpp *.hpp)

SET(LAUNCHER_CODENAME "${PROJECT_NAME}-launcher")

ADD_SUBDIRECTORY(launcher)

## Setup target ################################################################
ADD_LIBRARY(Common STATIC ${SRCS})

PKG_CHECK_MODULES(COMMON_DEPS REQUIRED glib-2.0)

TARGET_COMPILE_DEFINITIONS(Common
    PRIVATE LAUNCHER_PATH="${LIBEXEC_DIR}/${LAUNCHER_CODENAME}"
)


INCLUDE_DIRECTORIES(SYSTEM ${COMMON_FOLDER} ${LIBS_FOLDER} ${COMMON_DEPS_INCLUDE_DIRS})

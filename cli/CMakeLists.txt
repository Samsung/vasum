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
SET(CLI_CODENAME "vsm")
ADD_EXECUTABLE(${CLI_CODENAME} ${cli_SRCS})

ADD_DEPENDENCIES(${CLI_CODENAME} Common ${PROJECT_NAME}-client CargoIpc)

## Readline detection ##########################################################

FIND_PATH(READLINE_INCLUDE_DIR readline/readline.h
  HINTS ${READLINE_ROOT_DIR} PATH_SUFFIXES include)
FIND_LIBRARY(READLINE_LIBRARY readline
  HINTS ${READLINE_ROOT_DIR} PATH_SUFFIXES lib)
FIND_LIBRARY(NCURSES_LIBRARY ncurses)   # readline depends on libncurses
MARK_AS_ADVANCED(READLINE_INCLUDE_DIR READLINE_LIBRARY NCURSES_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Readline DEFAULT_MSG
  READLINE_LIBRARY NCURSES_LIBRARY READLINE_INCLUDE_DIR)

SET(READLINE_INCLUDE_DIRS ${READLINE_INCLUDE_DIR})
SET(READLINE_LIBRARIES ${READLINE_LIBRARY} ${NCURSES_LIBRARY})
INCLUDE_DIRECTORIES(${READLINE_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${CLI_CODENAME} ${READLINE_LIBRARIES})

## Link libraries ##############################################################
INCLUDE_DIRECTORIES(${CLIENT_FOLDER})
INCLUDE_DIRECTORIES(${COMMON_FOLDER})
TARGET_LINK_LIBRARIES(${CLI_CODENAME} Common ${PROJECT_NAME}-client ${LIB_DEPS_LIBRARIES} CargoIpc)

CONFIGURE_FILE(support/vsm-completion.sh.in
               ${CMAKE_BINARY_DIR}/vsm-completion.sh
               @ONLY)

## Install #####################################################################
INSTALL(TARGETS ${CLI_CODENAME} DESTINATION bin)
INSTALL(FILES   ${CMAKE_BINARY_DIR}/vsm-completion.sh
        DESTINATION ${SYSCONF_INSTALL_DIR}/bash_completion.d)

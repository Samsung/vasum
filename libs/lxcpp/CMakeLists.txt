# Copyright (C) 2015 Samsung Electronics Co., Ltd All Rights Reserved
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# @file   CMakeLists.txt
# @author Mateusz Malicki (m.malicki2@samsung.com)
#

PROJECT(lxcpp)

MESSAGE(STATUS "")
MESSAGE(STATUS "Generating makefile for the liblxcpp...")
FILE(GLOB HEADERS       *.hpp)
FILE(GLOB HEADERS_UTILS    ${COMMON_FOLDER}/utils/fd-utils.hpp
                           ${COMMON_FOLDER}/utils/exception.hpp
                           ${COMMON_FOLDER}/utils/channel.hpp
                           ${COMMON_FOLDER}/utils/environment.hpp
                           ${COMMON_FOLDER}/utils/execute.hpp)
FILE(GLOB HEADERS_NETLINK  ${COMMON_FOLDER}/netlink/*.hpp)
FILE(GLOB HEADERS_COMMANDS commands/*.hpp)

FILE(GLOB SRCS          *.cpp *.hpp)
FILE(GLOB SRCS_UTILS    ${COMMON_FOLDER}/utils/fd-utils.cpp
                        ${COMMON_FOLDER}/utils/exception.cpp
                        ${COMMON_FOLDER}/utils/channel.cpp
                        ${COMMON_FOLDER}/utils/environment.cpp
                        ${COMMON_FOLDER}/utils/execute.cpp)
FILE(GLOB SRCS_NETLINK  ${COMMON_FOLDER}/netlink/*.cpp)
FILE(GLOB SRCS_COMMANDS commands/*.cpp)

SET(_LIB_VERSION_ "${VERSION}")
SET(_LIB_SOVERSION_ "0")
SET(PC_FILE "lib${PROJECT_NAME}.pc")

## Setup target ################################################################
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS} ${SRCS_UTILS} ${SRCS_NETLINK} ${SRCS_COMMANDS})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
    SOVERSION   ${_LIB_SOVERSION_}
    VERSION     ${_LIB_VERSION_}
)

## Link libraries ##############################################################
FIND_PACKAGE(Boost COMPONENTS system filesystem)
INCLUDE_DIRECTORIES(${LIBS_FOLDER} ${COMMON_FOLDER})
INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} ${Boost_LIBRARIES} Logger)

## Generate the pc file ########################################################
CONFIGURE_FILE(${PC_FILE}.in ${CMAKE_CURRENT_BINARY_DIR}/${PC_FILE} @ONLY)

## Install #####################################################################
INSTALL(FILES       ${CMAKE_CURRENT_BINARY_DIR}/${PC_FILE}
        DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)

INSTALL(TARGETS     ${PROJECT_NAME}
        DESTINATION ${LIB_INSTALL_DIR}
        COMPONENT   RuntimeLibraries)

INSTALL(FILES       ${HEADERS}
        DESTINATION ${INCLUDE_INSTALL_DIR}/lxcpp)
INSTALL(FILES       ${HEADERS_UTILS}
        DESTINATION ${INCLUDE_INSTALL_DIR}/lxcpp/utils)
INSTALL(FILES       ${HEADERS_NETLINK}
        DESTINATION ${INCLUDE_INSTALL_DIR}/lxcpp/netlink)
INSTALL(FILES       ${HEADERS_COMMANDS}
        DESTINATION ${INCLUDE_INSTALL_DIR}/lxcpp/commands)

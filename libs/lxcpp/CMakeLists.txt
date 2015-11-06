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

SET(GUARD_CODENAME "${PROJECT_NAME}-guard")
ADD_SUBDIRECTORY(guard)

SET(ATTACH_CODENAME "${PROJECT_NAME}-attach")
ADD_SUBDIRECTORY(attach)

MESSAGE(STATUS "")
MESSAGE(STATUS "Generating makefile for the liblxcpp...")
FILE(GLOB HEADERS *.hpp ${COMMON_FOLDER}/config.hpp)
FILE(GLOB HEADERS_UTILS    ${COMMON_FOLDER}/utils/channel.hpp)
FILE(GLOB HEADERS_CGROUPS  cgroups/*.hpp)
FILE(GLOB HEADERS_COMMANDS commands/*.hpp)

FILE(GLOB SRCS          *.cpp *.hpp)
FILE(GLOB SRCS_COMMANDS commands/*.cpp)
FILE(GLOB SRCS_CGROUPS  cgroups/*.cpp)

SET(_LIB_VERSION_ "${VERSION}")
SET(_LIB_SOVERSION_ "0")
SET(PC_FILE "lib${PROJECT_NAME}.pc")

## Setup target ################################################################
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS} ${SRCS_COMMANDS} ${SRCS_CGROUPS})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
    SOVERSION   ${_LIB_SOVERSION_}
    VERSION     ${_LIB_VERSION_}
)

TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME}
    PRIVATE GUARD_PATH="${LIBEXEC_DIR}/${GUARD_CODENAME}"
    PRIVATE ATTACH_PATH="${LIBEXEC_DIR}/${ATTACH_CODENAME}"
)

ADD_DEPENDENCIES(${PROJECT_NAME} Common Logger Cargo Ipc)

## Link libraries ##############################################################
FIND_PACKAGE(Boost COMPONENTS system filesystem ${USE_BOOST_REGEX})
PKG_CHECK_MODULES(LXCPP_DEPS REQUIRED glib-2.0)

INCLUDE_DIRECTORIES(${LIBS_FOLDER} ${COMMON_FOLDER})
INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIRS} ${LXCPP_DEPS_INCLUDE_DIRS} ${JSON_C_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} Common ${pkgs_LDFLAGS} util ${Boost_LIBRARIES} Logger Cargo Ipc)

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
INSTALL(FILES       ${HEADERS_CGROUPS}
        DESTINATION ${INCLUDE_INSTALL_DIR}/lxcpp/cgroups)
INSTALL(FILES       ${HEADERS_COMMANDS}
        DESTINATION ${INCLUDE_INSTALL_DIR}/lxcpp/commands)

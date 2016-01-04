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
# @author  Krzysztof Dynowski (k.dynowski@samsumg.com)
#

MESSAGE(STATUS "")
MESSAGE(STATUS "Generating makefile for the launcher...")
SET(LAUNCHER_CODENAME "${PROJECT_NAME}-launcher")

## Setup target ################################################################
ADD_EXECUTABLE(${LAUNCHER_CODENAME} launcher.cpp)

## Link libraries ##############################################################
FIND_PACKAGE(Boost REQUIRED COMPONENTS system filesystem)
PKG_CHECK_MODULES(LAUNCHER_DEPS REQUIRED glib-2.0)
INCLUDE_DIRECTORIES(${COMMON_FOLDER})
INCLUDE_DIRECTORIES(SYSTEM ${CARGO_UTILS_INCLUDE_DIRS} ${LAUNCHER_DEPS_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${LAUNCHER_CODENAME} ${CARGO_UTILS_LIBRARIES} ${Boost_LIBRARIES} ${LAUNCHER_DEPS_LIBRARIES} lxcpp)


## Install #####################################################################
INSTALL(TARGETS ${LAUNCHER_CODENAME} DESTINATION ${LIBEXEC_DIR})


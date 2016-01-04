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
# @author Jan Olszak (j.olszak@samsung.com)
#

MESSAGE(STATUS "")
MESSAGE(STATUS "Generating makefile for the Attach...")
FILE(GLOB      attach_SRCS *.cpp *.hpp)


## Setup target ################################################################
ADD_EXECUTABLE(${ATTACH_CODENAME} ${attach_SRCS})


## Link libraries ##############################################################
PKG_CHECK_MODULES(ATTACH_DEPS REQUIRED libLogger libcargo-fd)
INCLUDE_DIRECTORIES(${LIBS_FOLDER} ${COMMON_FOLDER})
INCLUDE_DIRECTORIES(SYSTEM ${CARGO_UTILS_INCLUDE_DIRS} ${ATTACH_DEPS_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${ATTACH_CODENAME} ${CARGO_UTILS_LIBRARIES} ${ATTACH_DEPS_LIBRARIES} lxcpp)


## Install #####################################################################
INSTALL(TARGETS ${ATTACH_CODENAME} DESTINATION ${LIBEXEC_DIR})

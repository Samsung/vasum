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

MESSAGE ("Generating makefile for the Server...")
FILE(GLOB_RECURSE project_SRCS *.cpp *.cxx *.cc *.C *.c *.h *.hpp)
MESSAGE("Files: " ${project_SRCS})

## Setup target ################################################################
SET(SERVER_CODENAME "${PROJECT_NAME}-server")
ADD_EXECUTABLE(${SERVER_CODENAME} ${project_SRCS} )

## Link libraries ##############################################################
FIND_PACKAGE (Boost COMPONENTS program_options REQUIRED)
MESSAGE ("Include dirs of boost: " ${Boost_INCLUDE_DIRS} )
MESSAGE ("Libs of boost:         " ${Boost_LIBRARIES} )

PKG_CHECK_MODULES(SERVER_DEPS REQUIRED libvirt json)
INCLUDE_DIRECTORIES(SYSTEM ${SERVER_DEPS_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${SERVER_CODENAME} ${SERVER_DEPS_LIBRARIES} ${Boost_LIBRARIES})

## Install #####################################################################
INSTALL(TARGETS ${SERVER_CODENAME} DESTINATION bin)
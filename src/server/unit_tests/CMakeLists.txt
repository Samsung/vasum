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

MESSAGE ("Generating makefile for the Server unit tests...")
FILE(GLOB_RECURSE project_SRCS *.cpp)
FILE(GLOB src_SRCS ${SERVER_FOLDER}/src/scs-container.cpp
                   ${SERVER_FOLDER}/src/scs-configuration.cpp)

MESSAGE("Files: " ${project_SRCS} ${src_SRCS})

## Setup target ################################################################
SET(UT_SERVER_CODENAME "${PROJECT_NAME}-server-unit-tests")
ADD_EXECUTABLE(${UT_SERVER_CODENAME} ${project_SRCS} ${src_SRCS})

## Link libraries ##############################################################
FIND_PACKAGE (Boost COMPONENTS unit_test_framework REQUIRED)
MESSAGE ("Include dirs of boost: " ${Boost_INCLUDE_DIRS} )
MESSAGE ("Libs of boost:         " ${Boost_LIBRARIES} )

PKG_CHECK_MODULES(UT_SERVER_DEPS REQUIRED libvirt json)

INCLUDE_DIRECTORIES(SYSTEM ${UT_SERVER_DEPS_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${UT_SERVER_CODENAME} ${UT_SERVER_DEPS_LIBRARIES} ${Boost_LIBRARIES})

## Install #####################################################################
INSTALL(TARGETS ${UT_SERVER_CODENAME} DESTINATION bin)
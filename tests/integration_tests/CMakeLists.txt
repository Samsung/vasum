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
# @author Lukasz Kostyra (l.kostyra@samsung.com)
#

MESSAGE(STATUS "Generating makefile for Integration Tests...")


## Defines for further package installation ####################################
SET(TEST_DEST_DIR "${PYTHON_SITELIB}/sc_integration_tests")


## Search for .py files available ##############################################
FILE(GLOB py_SCRIPTS *.py)
FILE(GLOB main_SCRIPT sc_int_tests.py)
LIST(REMOVE_ITEM py_SCRIPTS ${main_SCRIPT})


## Install #####################################################################
INSTALL(FILES ${py_SCRIPTS} DESTINATION ${TEST_DEST_DIR})
INSTALL(PROGRAMS ${main_SCRIPT} DESTINATION ${SCRIPT_INSTALL_DIR})


## Subdirectories ##############################################################
ADD_SUBDIRECTORY(common)
ADD_SUBDIRECTORY(image_tests)

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

MESSAGE(STATUS "Including common package for Integration Tests...")

SET(TEST_COMMON_DEST_DIR "${TEST_DEST_DIR}/common")

FILE(GLOB common_SCRIPTS *.py)

INSTALL(FILES ${common_SCRIPTS} DESTINATION ${TEST_COMMON_DEST_DIR})


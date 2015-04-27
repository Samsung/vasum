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

MESSAGE(STATUS "Installing configs for the Client Unit Tests to " ${VSM_TEST_CONFIG_INSTALL_DIR})

## Generate ####################################################################
CONFIGURE_FILE(ut-client/test-daemon.conf.in
              ${CMAKE_BINARY_DIR}/ut-client/test-daemon.conf @ONLY)
FILE(GLOB client_manager_CONF_GEN ${CMAKE_BINARY_DIR}/ut-client/*.conf)

CONFIGURE_FILE(ut-client/templates/console-dbus.conf.in
               ${CMAKE_BINARY_DIR}/ut-client/templates/console-dbus.conf @ONLY)
CONFIGURE_FILE(ut-client/templates/console-ipc.conf.in
               ${CMAKE_BINARY_DIR}/ut-client/templates/console-ipc.conf @ONLY)
FILE(GLOB client_templates_CONF_GEN ${CMAKE_BINARY_DIR}/ut-client/templates/*.conf)

## Install #####################################################################
INSTALL(FILES        ${client_manager_CONF_GEN}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/client/ut-client)
INSTALL(FILES        ${client_templates_CONF_GEN}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/client/ut-client/templates)

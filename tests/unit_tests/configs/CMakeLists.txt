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
# @author Dariusz Michaluk (d.michaluk@samsung.com)
#

MESSAGE(STATUS "Installing configs for the Unit Tests to " ${VSM_TEST_CONFIG_INSTALL_DIR})

FILE(GLOB ut_zone_CONF            *.conf)
FILE(GLOB ut_zone_templates_CONF  templates/*.conf)
FILE(GLOB ut_lxc_templates_CONF   templates/*.sh)

## Generate ####################################################################
CONFIGURE_FILE(test-daemon.conf.in
              ${CMAKE_BINARY_DIR}/test-daemon.conf @ONLY)
FILE(GLOB ut_zone_CONF_GEN ${CMAKE_BINARY_DIR}/*.conf)

CONFIGURE_FILE(templates/console-dbus.conf.in
               ${CMAKE_BINARY_DIR}/templates/console-dbus.conf @ONLY)
CONFIGURE_FILE(templates/console-ipc.conf.in
               ${CMAKE_BINARY_DIR}/templates/console-ipc.conf @ONLY)
FILE(GLOB ut_zone_templates_CONF_GEN ${CMAKE_BINARY_DIR}/templates/*.conf)


## Install #####################################################################
INSTALL(FILES        ${ut_zone_CONF}
                     ${ut_zone_CONF_GEN}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR})

INSTALL(FILES        ${ut_zone_templates_CONF}
                     ${ut_zone_templates_CONF_GEN}
        DESTINATION  ${VSM_TEST_TEMPLATES_INSTALL_DIR})

INSTALL(PROGRAMS     ${ut_lxc_templates_CONF}
        DESTINATION  ${VSM_TEST_TEMPLATES_INSTALL_DIR})

INSTALL(FILES        dbus/ut-dbus.conf
                     dbus/ut-dbus-system.conf
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/dbus)

INSTALL(FILES        provision/test-provision.conf
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/provision)

INSTALL(FILES        utils/file.txt
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/utils)

INSTALL(FILES        systemd/vasum-socket-test.socket
                     systemd/vasum-socket-test.service
        DESTINATION  ${SYSTEMD_UNIT_DIR})

INSTALL(FILES        dbus-1/system.d/org.tizen.vasum.tests.conf
        DESTINATION  ${SYSCONF_INSTALL_DIR}/dbus-1/system.d/)


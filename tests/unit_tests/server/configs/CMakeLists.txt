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

MESSAGE(STATUS "Installing configs for the Server Unit Tests to " ${VSM_TEST_CONFIG_INSTALL_DIR})

FILE(GLOB server_manager_CONF       ut-server/*.conf)
FILE(GLOB server_templates_CONF     ut-server/templates/*.conf)

FILE(GLOB manager_manager_CONF      ut-zones-manager/*.conf)
FILE(GLOB manager_templates_CONF    ut-zones-manager/templates/*.conf)

FILE(GLOB zone_CONF            ut-zone/*.conf)
FILE(GLOB zone_templates_CONF  ut-zone/templates/*.conf)

FILE(GLOB zone_provision_CONF  ut-zone-provision/*.conf)

FILE(GLOB admin_templates_CONF      ut-zone-admin/templates/*.conf)

FILE(GLOB connection_CONF           ut-zone-connection/*.conf)


## Generate ####################################################################
CONFIGURE_FILE(ut-server/test-daemon.conf.in
              ${CMAKE_BINARY_DIR}/ut-server/test-daemon.conf @ONLY)
FILE(GLOB server_manager_CONF_GEN ${CMAKE_BINARY_DIR}/ut-server/*.conf)

CONFIGURE_FILE(ut-zone/templates/test-dbus.conf.in
               ${CMAKE_BINARY_DIR}/ut-zone/templates/test-dbus.conf @ONLY)
FILE(GLOB zone_templates_CONF_GEN ${CMAKE_BINARY_DIR}/ut-zone/templates/*.conf)

CONFIGURE_FILE(ut-zones-manager/test-daemon.conf.in
               ${CMAKE_BINARY_DIR}/ut-zones-manager/test-daemon.conf @ONLY)
FILE(GLOB manager_manager_CONF_GEN ${CMAKE_BINARY_DIR}/ut-zones-manager/*.conf)

CONFIGURE_FILE(ut-zones-manager/templates/console-dbus.conf.in
               ${CMAKE_BINARY_DIR}/ut-zones-manager/templates/console-dbus.conf @ONLY)
FILE(GLOB manager_templates_CONF_GEN ${CMAKE_BINARY_DIR}/ut-zones-manager/templates/*.conf)


## Install #####################################################################
INSTALL(FILES        ${server_manager_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-server)
INSTALL(FILES        ${server_manager_CONF_GEN}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-server)
INSTALL(FILES        ${server_templates_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-server/templates)

INSTALL(FILES        ${manager_manager_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zones-manager)
INSTALL(FILES        ${manager_manager_CONF_GEN}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zones-manager)
INSTALL(FILES        ${manager_templates_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zones-manager/templates)
INSTALL(FILES        ${manager_templates_CONF_GEN}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zones-manager/templates)

INSTALL(FILES        ${zone_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zone)
INSTALL(FILES        ${zone_templates_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zone/templates)
INSTALL(FILES        ${zone_templates_CONF_GEN}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zone/templates)

INSTALL(FILES        ${zone_provision_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zone-provision)

INSTALL(FILES        ${admin_templates_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zone-admin/templates)

INSTALL(FILES        ${connection_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zone-connection)

INSTALL(FILES        dbus-1/system.d/org.tizen.vasum.tests.conf
        DESTINATION  ${SYSCONF_INSTALL_DIR}/dbus-1/system.d/)

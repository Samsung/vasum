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
FILE(GLOB server_zone_CONF     ut-server/zones/*.conf)

FILE(GLOB manager_manager_CONF      ut-zones-manager/*.conf)
FILE(GLOB manager_zone_CONF    ut-zones-manager/zones/*.conf)

FILE(GLOB zone_CONF            ut-zone/*.conf)
FILE(GLOB zone_zone_CONF  ut-zone/zones/*.conf)

FILE(GLOB admin_zone_CONF      ut-zone-admin/zones/*.conf)

FILE(GLOB connection_CONF           ut-zone-connection/*.conf)


## Generate ####################################################################
CONFIGURE_FILE(ut-server/test-daemon.conf.in
              ${CMAKE_BINARY_DIR}/ut-server/test-daemon.conf @ONLY)
CONFIGURE_FILE(ut-server/buggy-daemon.conf.in
              ${CMAKE_BINARY_DIR}/ut-server/buggy-daemon.conf @ONLY)
FILE(GLOB server_manager_CONF_GEN ${CMAKE_BINARY_DIR}/ut-server/*.conf)

CONFIGURE_FILE(ut-zone/zones/test-dbus.conf.in
               ${CMAKE_BINARY_DIR}/ut-zone/zones/test-dbus.conf @ONLY)
FILE(GLOB zone_zone_CONF_GEN ${CMAKE_BINARY_DIR}/ut-zone/zones/*.conf)

CONFIGURE_FILE(ut-zones-manager/test-daemon.conf.in
               ${CMAKE_BINARY_DIR}/ut-zones-manager/test-daemon.conf @ONLY)
CONFIGURE_FILE(ut-zones-manager/buggy-daemon.conf.in
               ${CMAKE_BINARY_DIR}/ut-zones-manager/buggy-daemon.conf @ONLY)
CONFIGURE_FILE(ut-zones-manager/buggy-default-daemon.conf.in
               ${CMAKE_BINARY_DIR}/ut-zones-manager/buggy-default-daemon.conf @ONLY)
CONFIGURE_FILE(ut-zones-manager/buggy-foreground-daemon.conf.in
               ${CMAKE_BINARY_DIR}/ut-zones-manager/buggy-foreground-daemon.conf @ONLY)
CONFIGURE_FILE(ut-zones-manager/test-dbus-daemon.conf.in
               ${CMAKE_BINARY_DIR}/ut-zones-manager/test-dbus-daemon.conf @ONLY)
CONFIGURE_FILE(ut-zones-manager/empty-dbus-daemon.conf.in
               ${CMAKE_BINARY_DIR}/ut-zones-manager/empty-dbus-daemon.conf @ONLY)
FILE(GLOB manager_manager_CONF_GEN ${CMAKE_BINARY_DIR}/ut-zones-manager/*.conf)

CONFIGURE_FILE(ut-zones-manager/zones/console1-dbus.conf.in
               ${CMAKE_BINARY_DIR}/ut-zones-manager/zones/console1-dbus.conf @ONLY)
CONFIGURE_FILE(ut-zones-manager/zones/console2-dbus.conf.in
               ${CMAKE_BINARY_DIR}/ut-zones-manager/zones/console2-dbus.conf @ONLY)
CONFIGURE_FILE(ut-zones-manager/zones/console3-dbus.conf.in
               ${CMAKE_BINARY_DIR}/ut-zones-manager/zones/console3-dbus.conf @ONLY)
FILE(GLOB manager_zone_CONF_GEN ${CMAKE_BINARY_DIR}/ut-zones-manager/zones/*.conf)

CONFIGURE_FILE(ut-zones-manager/templates/template.conf.in
               ${CMAKE_BINARY_DIR}/ut-zones-manager/templates/template.conf @ONLY)
FILE(GLOB manager_zone_TEMPLATE_GEN ${CMAKE_BINARY_DIR}/ut-zones-manager/templates/*.conf)


## Install #####################################################################
INSTALL(FILES        ${server_manager_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-server)
INSTALL(FILES        ${server_manager_CONF_GEN}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-server)
INSTALL(FILES        ${server_zone_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-server/zones)

INSTALL(FILES        ${manager_manager_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zones-manager)
INSTALL(FILES        ${manager_manager_CONF_GEN}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zones-manager)
INSTALL(FILES        ${manager_zone_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zones-manager/zones)
INSTALL(FILES        ${manager_zone_CONF_GEN}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zones-manager/zones)
INSTALL(FILES        ${manager_zone_TEMPLATE_GEN}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zones-manager/templates)

INSTALL(FILES        ${zone_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zone)
INSTALL(FILES        ${zone_zone_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zone/zones)
INSTALL(FILES        ${zone_zone_CONF_GEN}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zone/zones)

INSTALL(FILES        ${admin_zone_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zone-admin/zones)

INSTALL(FILES        ${connection_CONF}
        DESTINATION  ${VSM_TEST_CONFIG_INSTALL_DIR}/server/ut-zone-connection)

INSTALL(FILES        dbus-1/system.d/org.tizen.vasum.tests.conf
        DESTINATION  ${SYSCONF_INSTALL_DIR}/dbus-1/system.d/)

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

MESSAGE(STATUS "Installing configs for the Server Unit Tests to " ${SC_TEST_CONFIG_INSTALL_DIR})

FILE(GLOB server_manager_CONF       ut-server/*.conf)
FILE(GLOB server_container_CONF     ut-server/containers/*.conf)

FILE(GLOB manager_manager_CONF      ut-containers-manager/*.conf)
FILE(GLOB manager_container_CONF    ut-containers-manager/containers/*.conf)
FILE(GLOB manager_admin_TEMPLATE    ut-containers-manager/templates/*.conf)

FILE(GLOB container_CONF            ut-container/*.conf)
FILE(GLOB container_container_CONF  ut-container/containers/*.conf)

FILE(GLOB admin_container_CONF      ut-container-admin/containers/*.conf)

FILE(GLOB network_container_CONF    ut-network-admin/containers/*.conf)

FILE(GLOB connection_CONF           ut-container-connection/*.conf)


## Generate ####################################################################
CONFIGURE_FILE(ut-server/test-daemon.conf.in
              ${CMAKE_BINARY_DIR}/ut-server/test-daemon.conf @ONLY)
CONFIGURE_FILE(ut-server/buggy-daemon.conf.in
              ${CMAKE_BINARY_DIR}/ut-server/buggy-daemon.conf @ONLY)
FILE(GLOB server_manager_CONF_GEN ${CMAKE_BINARY_DIR}/ut-server/*.conf)

CONFIGURE_FILE(ut-container-admin/containers/buggy.conf.in
               ${CMAKE_BINARY_DIR}/ut-container-admin/containers/buggy.conf @ONLY)
CONFIGURE_FILE(ut-container-admin/containers/test.conf.in
               ${CMAKE_BINARY_DIR}/ut-container-admin/containers/test.conf @ONLY)
CONFIGURE_FILE(ut-container-admin/containers/test-no-shutdown.conf.in
               ${CMAKE_BINARY_DIR}/ut-container-admin/containers/test-no-shutdown.conf @ONLY)
FILE(GLOB admin_container_CONF_GEN ${CMAKE_BINARY_DIR}/ut-container-admin/containers/*.conf)

CONFIGURE_FILE(ut-network-admin/containers/test.conf.in
               ${CMAKE_BINARY_DIR}/ut-network-admin/containers/test.conf @ONLY)
CONFIGURE_FILE(ut-network-admin/containers/buggy.conf.in
               ${CMAKE_BINARY_DIR}/ut-network-admin/containers/buggy.conf @ONLY)
FILE(GLOB network_container_CONF_GEN ${CMAKE_BINARY_DIR}/ut-network-admin/containers/*.conf)

CONFIGURE_FILE(ut-container/containers/test-dbus.conf.in
               ${CMAKE_BINARY_DIR}/ut-container/containers/test-dbus.conf @ONLY)
FILE(GLOB container_container_CONF_GEN ${CMAKE_BINARY_DIR}/ut-container/containers/*.conf)

CONFIGURE_FILE(ut-containers-manager/test-daemon.conf.in
               ${CMAKE_BINARY_DIR}/ut-containers-manager/test-daemon.conf @ONLY)
CONFIGURE_FILE(ut-containers-manager/buggy-daemon.conf.in
               ${CMAKE_BINARY_DIR}/ut-containers-manager/buggy-daemon.conf @ONLY)
CONFIGURE_FILE(ut-containers-manager/buggy-default-daemon.conf.in
               ${CMAKE_BINARY_DIR}/ut-containers-manager/buggy-default-daemon.conf @ONLY)
CONFIGURE_FILE(ut-containers-manager/buggy-foreground-daemon.conf.in
               ${CMAKE_BINARY_DIR}/ut-containers-manager/buggy-foreground-daemon.conf @ONLY)
CONFIGURE_FILE(ut-containers-manager/test-dbus-daemon.conf.in
               ${CMAKE_BINARY_DIR}/ut-containers-manager/test-dbus-daemon.conf @ONLY)
FILE(GLOB manager_manager_CONF_GEN ${CMAKE_BINARY_DIR}/ut-containers-manager/*.conf)

CONFIGURE_FILE(ut-containers-manager/containers/console1-dbus.conf.in
               ${CMAKE_BINARY_DIR}/ut-containers-manager/containers/console1-dbus.conf @ONLY)
CONFIGURE_FILE(ut-containers-manager/containers/console2-dbus.conf.in
               ${CMAKE_BINARY_DIR}/ut-containers-manager/containers/console2-dbus.conf @ONLY)
CONFIGURE_FILE(ut-containers-manager/containers/console3-dbus.conf.in
               ${CMAKE_BINARY_DIR}/ut-containers-manager/containers/console3-dbus.conf @ONLY)
FILE(GLOB manager_container_CONF_GEN ${CMAKE_BINARY_DIR}/ut-containers-manager/containers/*.conf)


## Install #####################################################################
INSTALL(FILES        ${server_manager_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-server)
INSTALL(FILES        ${server_manager_CONF_GEN}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-server)
INSTALL(FILES        ${server_container_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-server/containers)

INSTALL(FILES        ${manager_manager_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-containers-manager)
INSTALL(FILES        ${manager_manager_CONF_GEN}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-containers-manager)
INSTALL(FILES        ${manager_container_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-containers-manager/containers)
INSTALL(FILES        ${manager_container_CONF_GEN}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-containers-manager/containers)
INSTALL(FILES        ${manager_admin_TEMPLATE}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-containers-manager/templates)

INSTALL(FILES        ${container_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container)
INSTALL(FILES        ${container_container_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container/containers)
INSTALL(FILES        ${container_container_CONF_GEN}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container/containers)

INSTALL(FILES        ${admin_container_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container-admin/containers)
INSTALL(FILES        ${admin_container_CONF_GEN}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container-admin/containers)

INSTALL(FILES        ${network_container_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-network-admin/containers)
INSTALL(FILES        ${network_container_CONF_GEN}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-network-admin/containers)

INSTALL(FILES        ${connection_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container-connection)

INSTALL(FILES        dbus-1/system.d/org.tizen.containers.tests.conf
        DESTINATION  ${SYSCONF_INSTALL_DIR}/dbus-1/system.d/)

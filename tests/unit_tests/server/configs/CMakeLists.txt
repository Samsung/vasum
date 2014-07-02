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

MESSAGE(STATUS "Installing configs for the Server Unit Tests to " ${SC_CONFIG_INSTALL_DIR})

FILE(GLOB server_manager_CONF       ut-server/*.conf)
FILE(GLOB server_container_CONF     ut-server/containers/*.conf)
FILE(GLOB server_admin_CONF         ut-server/libvirt-config/*.xml)

FILE(GLOB manager_manager_CONF      ut-containers-manager/*.conf)
FILE(GLOB manager_container_CONF    ut-containers-manager/containers/*.conf)
FILE(GLOB manager_admin_CONF        ut-containers-manager/libvirt-config/*.xml)

FILE(GLOB container_CONF            ut-container/*.conf)
FILE(GLOB container_container_CONF  ut-container/containers/*.conf)
FILE(GLOB container_admin_CONF      ut-container/libvirt-config/*.xml)

FILE(GLOB admin_container_CONF      ut-container-admin/containers/*.conf)
FILE(GLOB admin_admin_CONF          ut-container-admin/libvirt-config/*.xml)

FILE(GLOB network_container_CONF    ut-network-admin/containers/*.conf)
FILE(GLOB network_admin_CONF        ut-network-admin/libvirt-config/*.xml)

FILE(GLOB connection_CONF           ut-container-connection/*.conf)


## Generate ####################################################################
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

CONFIGURE_FILE(ut-containers-manager/libvirt-config/console1-dbus.xml.in
               ${CMAKE_BINARY_DIR}/ut-containers-manager/libvirt-config/console1-dbus.xml @ONLY)
CONFIGURE_FILE(ut-containers-manager/libvirt-config/console2-dbus.xml.in
               ${CMAKE_BINARY_DIR}/ut-containers-manager/libvirt-config/console2-dbus.xml @ONLY)
CONFIGURE_FILE(ut-containers-manager/libvirt-config/console3-dbus.xml.in
               ${CMAKE_BINARY_DIR}/ut-containers-manager/libvirt-config/console3-dbus.xml @ONLY)
FILE(GLOB manager_admin_CONF_GEN ${CMAKE_BINARY_DIR}/ut-containers-manager/libvirt-config/*.xml)

CONFIGURE_FILE(ut-container/libvirt-config/test-dbus.xml.in
               ${CMAKE_BINARY_DIR}/ut-container/libvirt-config/test-dbus.xml @ONLY)
FILE(GLOB container_admin_CONF_GEN ${CMAKE_BINARY_DIR}/ut-container/libvirt-config/*.xml)


## Install #####################################################################
INSTALL(FILES        ${server_manager_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-server)
INSTALL(FILES        ${server_container_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-server/containers)
INSTALL(FILES        ${server_admin_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-server/libvirt-config)

INSTALL(FILES        ${manager_manager_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-containers-manager)
INSTALL(FILES        ${manager_container_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-containers-manager/containers)
INSTALL(FILES        ${manager_admin_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-containers-manager/libvirt-config)
INSTALL(FILES        ${manager_admin_CONF_GEN}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-containers-manager/libvirt-config)

INSTALL(FILES        ${container_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container)
INSTALL(FILES        ${container_container_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container/containers)
INSTALL(FILES        ${container_admin_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container/libvirt-config)
INSTALL(FILES        ${container_admin_CONF_GEN}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container/libvirt-config)

INSTALL(FILES        ${admin_container_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container-admin/containers)
INSTALL(FILES        ${admin_container_CONF_GEN}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container-admin/containers)
INSTALL(FILES        ${admin_admin_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container-admin/libvirt-config)

INSTALL(FILES        ${network_container_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-network-admin/containers)
INSTALL(FILES        ${network_container_CONF_GEN}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-network-admin/containers)
INSTALL(FILES        ${network_admin_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-network-admin/libvirt-config)

INSTALL(FILES        ${connection_CONF}
        DESTINATION  ${SC_TEST_CONFIG_INSTALL_DIR}/server/ut-container-connection)

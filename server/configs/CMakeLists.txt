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
# @author Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
#

MESSAGE(STATUS "Installing configs to " ${VSM_CONFIG_INSTALL_DIR})

FILE(GLOB zone_templates_CONF  templates/*.conf)
FILE(GLOB lxc_templates_CONF   templates/*.sh)

## Generate ####################################################################
IF(NOT WITHOUT_SYSTEMD)
CONFIGURE_FILE(systemd/vasum.service.in
               ${CMAKE_BINARY_DIR}/systemd/vasum.service)
CONFIGURE_FILE(systemd/vasum.socket.in
               ${CMAKE_BINARY_DIR}/systemd/vasum.socket)
ENDIF(NOT WITHOUT_SYSTEMD)

CONFIGURE_FILE(daemon.conf.in
               ${CMAKE_BINARY_DIR}/daemon.conf)

IF(NOT WITHOUT_DBUS)
CONFIGURE_FILE(dbus-1/system.d/org.tizen.vasum.host.conf.in
               ${CMAKE_BINARY_DIR}/dbus-1/system.d/org.tizen.vasum.host.conf)
ENDIF(NOT WITHOUT_DBUS)

## Install #####################################################################
INSTALL(FILES       ${CMAKE_BINARY_DIR}/daemon.conf
        DESTINATION ${VSM_CONFIG_INSTALL_DIR})

IF(NOT WITHOUT_DBUS)
INSTALL(FILES       ${CMAKE_BINARY_DIR}/dbus-1/system.d/org.tizen.vasum.host.conf
        DESTINATION ${SYSCONF_INSTALL_DIR}/dbus-1/system.d/)
ENDIF(NOT WITHOUT_DBUS)

INSTALL(DIRECTORY   DESTINATION ${DATA_DIR}/zones)

INSTALL(FILES       ${zone_templates_CONF}
        DESTINATION ${VSM_CONFIG_INSTALL_DIR}/templates)

INSTALL(PROGRAMS    ${lxc_templates_CONF}
        DESTINATION ${VSM_CONFIG_INSTALL_DIR}/templates)

IF(NOT WITHOUT_SYSTEMD)
INSTALL(FILES
        ${CMAKE_BINARY_DIR}/systemd/vasum.service
        ${CMAKE_BINARY_DIR}/systemd/vasum.socket
        DESTINATION ${SYSTEMD_UNIT_DIR})
ENDIF(NOT WITHOUT_SYSTEMD)


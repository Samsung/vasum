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

FILE(GLOB zone_CONF          zones/*.conf)
FILE(GLOB admin_CONF         lxc-templates/*.sh)
FILE(GLOB template_CONF      templates/*.conf)

## Generate ####################################################################
CONFIGURE_FILE(systemd/vasum.service.in
               ${CMAKE_BINARY_DIR}/systemd/vasum.service)


## Install #####################################################################
INSTALL(FILES       daemon.conf
        DESTINATION ${VSM_CONFIG_INSTALL_DIR})

# preprocess d-bus configs
CONFIGURE_FILE(dbus-1/system.d/org.tizen.vasum.host.conf.in
               ${CMAKE_BINARY_DIR}/dbus-1/system.d/org.tizen.vasum.host.conf)

INSTALL(FILES       ${CMAKE_BINARY_DIR}/dbus-1/system.d/org.tizen.vasum.host.conf
        DESTINATION ${SYSCONF_INSTALL_DIR}/dbus-1/system.d/)

INSTALL(FILES       ${zone_CONF}
        DESTINATION ${VSM_CONFIG_INSTALL_DIR}/zones)

INSTALL(PROGRAMS    ${admin_CONF}
        DESTINATION ${VSM_CONFIG_INSTALL_DIR}/lxc-templates)

INSTALL(PROGRAMS    ${template_CONF}
        DESTINATION ${VSM_CONFIG_INSTALL_DIR}/templates)

INSTALL(FILES       ${CMAKE_BINARY_DIR}/systemd/vasum.service
        DESTINATION ${SYSTEMD_UNIT_DIR})

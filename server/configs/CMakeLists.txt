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

MESSAGE(STATUS "Installing configs to " ${SC_CONFIG_INSTALL_DIR})

FILE(GLOB container_CONF     containers/*.conf)
FILE(GLOB admin_CONF         libvirt-config/*.xml)


## Generate ####################################################################
CONFIGURE_FILE(systemd/security-containers.service.in
               ${CMAKE_BINARY_DIR}/systemd/security-containers.service)


## Install #####################################################################
INSTALL(FILES       daemon.conf
        DESTINATION ${SC_CONFIG_INSTALL_DIR})

# preprocess d-bus configs
CONFIGURE_FILE(dbus-1/system.d/org.tizen.containers.host.conf.in
               ${CMAKE_BINARY_DIR}/dbus-1/system.d/org.tizen.containers.host.conf)
CONFIGURE_FILE(image-skel/etc/dbus-1/system.d/org.tizen.containers.domain.conf.in
               ${CMAKE_BINARY_DIR}/image-skel/etc/dbus-1/system.d/org.tizen.containers.domain.conf)

# TODO This file should be installed to /etc/dbus-1/system.d/ on an every container
INSTALL(FILES       ${CMAKE_BINARY_DIR}/image-skel/etc/dbus-1/system.d/org.tizen.containers.domain.conf
        DESTINATION ${SC_CONFIG_INSTALL_DIR}/image-skel/etc/dbus-1/system.d)

INSTALL(FILES       ${CMAKE_BINARY_DIR}/dbus-1/system.d/org.tizen.containers.host.conf
        DESTINATION ${SYSCONF_INSTALL_DIR}/dbus-1/system.d/)

INSTALL(FILES       ${container_CONF}
        DESTINATION ${SC_CONFIG_INSTALL_DIR}/containers)

INSTALL(FILES       ${admin_CONF}
        DESTINATION ${SC_CONFIG_INSTALL_DIR}/libvirt-config)

INSTALL(FILES       ${CMAKE_BINARY_DIR}/systemd/security-containers.service
        DESTINATION ${SYSTEMD_UNIT_DIR})

#!/bin/bash

#  template script for creating Fedora LXC container
#  This script is a wrapper for the lxc-fedora template
#
#  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
#
#  Contact: Dariusz Michaluk  <d.michaluk@samsung.com>
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

usage()
{
    cat <<EOF
usage:
    $1 -n|--name=<zone_name>
        [-p|--path=<path>] [--rootfs=<rootfs>] [--vt=<vt>]
        [--ipv4=<ipv4>] [--ipv4-gateway=<ipv4_gateway>] [-h|--help]
Mandatory args:
  -n,--name         zone name
  -p,--path         path to zone config files
  --rootfs          path to zone rootfs
Optional args:
  --vt              zone virtual terminal
  --ipv4            zone IP address
  --ipv4-gateway    zone gateway
  -h,--help         print help
EOF
    return 0
}

options=$(getopt -o hp:n: -l help,rootfs:,path:,vt:,name:,ipv4:,ipv4-gateway: -- "$@")
if [ $? -ne 0 ]; then
    usage $(basename $0)
    exit 1
fi
eval set -- "$options"

while true
do
    case "$1" in
        -h|--help)      usage $(basename $0) && exit 0;;
        --rootfs)       rootfs=$2; shift 2;;
        -p|--path)      path=$2; shift 2;;
        --vt)           vt=$2; shift 2;;
        -n|--name)      name=$2; shift 2;;
        --ipv4)         ipv4=$2; shift 2;;
        --ipv4-gateway) ipv4_gateway=$2; shift 2;;
        --)             shift 1; break ;;
        *)              break ;;
    esac
done

if [ "$(id -u)" != "0" ]; then
    echo "This script should be run as 'root'"
    exit 1
fi

if [ -z $name ]; then
    echo "Zone name must be given"
    exit 1
fi

if [ -z "$path" ]; then
    echo "'path' parameter is required"
    exit 1
fi

if [ -z "$rootfs" ]; then
    echo "'rootfs' parameter is required"
    exit 1
fi

/usr/share/lxc/templates/lxc-fedora --name="$name" --path="$path" --rootfs="$rootfs"

if [ "$vt" -gt "0" ]; then
    echo "Setup Desktop"
    chroot $rootfs yum -y --nogpgcheck groupinstall "LXDE Desktop"
    chroot $rootfs yum -y --nogpgcheck groupinstall "Firefox Web Browser"
    chroot $rootfs yum -y --nogpgcheck install openarena
    chroot $rootfs yum -y --nogpgcheck install http://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm \
                                               http://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm
    chroot $rootfs yum -y --nogpgcheck install gstreamer1-libav gstreamer1-vaapi gstreamer1-plugins-{good,good-extras,ugly}
    chroot $rootfs yum -y remove pulseaudio
    chroot $rootfs passwd -d root

    cat <<EOF >>${rootfs}/etc/X11/xorg.conf
Section "ServerFlags"
  Option "AutoAddDevices" "false"
EndSection
Section "InputDevice"
  Identifier "event0"
  Driver "evdev"
  Option "Device" "/dev/input/event0"
  Option "AutoServerLayout" "true"
EndSection
Section "InputDevice"
  Identifier "event1"
  Driver "evdev"
  Option "Device" "/dev/input/event1"
  Option "AutoServerLayout" "true"
EndSection
Section "InputDevice"
  Identifier "event10"
  Driver "evdev"
  Option "Device" "/dev/input/event10"
  Option "AutoServerLayout" "true"
EndSection
Section "InputDevice"
  Identifier "event2"
  Driver "evdev"
  Option "Device" "/dev/input/event2"
  Option "AutoServerLayout" "true"
EndSection
Section "InputDevice"
  Identifier "event3"
  Driver "evdev"
  Option "Device" "/dev/input/event3"
  Option "AutoServerLayout" "true"
EndSection
Section "InputDevice"
  Identifier "event4"
  Driver "evdev"
  Option "Device" "/dev/input/event4"
  Option "AutoServerLayout" "true"
EndSection
Section "InputDevice"
  Identifier "event5"
  Driver "evdev"
  Option "Device" "/dev/input/event5"
  Option "AutoServerLayout" "true"
EndSection
Section "InputDevice"
  Identifier "event6"
  Driver "evdev"
  Option "Device" "/dev/input/event6"
  Option "AutoServerLayout" "true"
EndSection
Section "InputDevice"
  Identifier "event7"
  Driver "evdev"
  Option "Device" "/dev/input/event7"
  Option "AutoServerLayout" "true"
EndSection
Section "InputDevice"
  Identifier "event8"
  Driver "evdev"
  Option "Device" "/dev/input/event8"
  Option "AutoServerLayout" "true"
EndSection
Section "InputDevice"
  Identifier "event9"
  Driver "evdev"
  Option "Device" "/dev/input/event9"
  Option "AutoServerLayout" "true"
EndSection
Section "InputDevice"
  Identifier "mice"
  Driver "evdev"
  Option "Device" "/dev/input/mice"
  Option "AutoServerLayout" "true"
EndSection
Section "InputDevice"
  Identifier "mouse0"
  Driver "evdev"
  Option "Device" "/dev/input/mouse0"
  Option "AutoServerLayout" "true"
EndSection
EOF

    chroot  ${rootfs} rm -rf /etc/systemd/system/display-manager.service
    cat <<EOF >${rootfs}/etc/systemd/system/display-manager.service
[Unit]
Description=Start Desktop

[Service]
ExecStartPre=/bin/mknod /dev/tty${vt} c 0x4 0x${vt}
ExecStart=/bin/startx -- :0 vt${vt}

[Install]
WantedBy=graphical.target
EOF

    chroot $rootfs ln -sf /dev/null /etc/systemd/system/initial-setup-graphical.service
    chroot $rootfs ln -sf /usr/lib/systemd/system/graphical.target /etc/systemd/system/default.target
    chroot $rootfs mkdir -p /etc/systemd/system/graphical.target.wants
    chroot $rootfs ln -sf /etc/systemd/system/display-manager.service /etc/systemd/system/graphical.target.wants/display-manager.service

    cat <<EOF >>${path}/config
lxc.mount.entry = /dev/dri dev/dri none bind,optional,create=dir
lxc.mount.entry = /dev/input dev/input none bind,optional,create=dir
lxc.mount.entry = /dev/snd dev/snd none bind,optional,create=dir
### /dev/dri/*
lxc.cgroup.devices.allow = c 226:* rwm
### /dev/input/*
lxc.cgroup.devices.allow = c 13:* rwm
### /dev/snd/*
lxc.cgroup.devices.allow = c 116:* rwm
### /dev/tty*
lxc.cgroup.devices.allow = c 4:* rwm
EOF

fi

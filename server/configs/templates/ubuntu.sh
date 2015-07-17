#!/bin/bash

#  template script for creating Ubuntu LXC container
#  This script is a wrapper for the lxc-ubuntu template
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
        -h|--help)      usage $0 && exit 0;;
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

/usr/share/lxc/templates/lxc-ubuntu --name="$name" --path="$path" --rootfs="$rootfs"

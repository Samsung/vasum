#!/bin/bash

echo LXC template, args: $@

options=$(getopt -o p:n: -l rootfs:,path:,name: -- "$@")
if [ $? -ne 0 ]; then
    exit 1
fi
eval set -- "$options"

while true
do
    case "$1" in
        -p|--path)      path=$2; shift 2;;
        --rootfs)       rootfs=$2; shift 2;;
        -n|--name)      name=$2; shift 2;;
        --)             shift 1; break ;;
        *)              break ;;
    esac
done

# XXX assume rootfs if mounted from iso

# Prepare container configuration file
> ${path}/config
cat <<EOF >> ${path}/config
lxc.utsname = ${name}
lxc.rootfs = ${rootfs}

# userns 1-to-1 mapping
#lxc.id_map = u 0 0 65536
#lxc.id_map = g 0 0 65536

lxc.pts = 256
lxc.tty = 0

lxc.mount.auto = proc sys cgroup
lxc.mount.entry = /var/run/containers/private/run var/run none rw,bind 0 0
EOF


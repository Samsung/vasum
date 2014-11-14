#!/bin/bash

echo UnitTest LXC template, args: $@

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

# Prepare container rootfs
ROOTFS_DIRS="\
${rootfs}/bin \
${rootfs}/dev \
${rootfs}/etc \
${rootfs}/home \
${rootfs}/lib \
${rootfs}/lib64 \
${rootfs}/proc \
${rootfs}/root \
${rootfs}/run \
${rootfs}/sbin \
${rootfs}/sys \
${rootfs}/tmp \
${rootfs}/usr \
${rootfs}/var \
${rootfs}/var/run
"
/bin/mkdir ${ROOTFS_DIRS}

# Prepare container configuration file
> ${path}/config
cat <<EOF >> ${path}/config
lxc.utsname = ${name}
lxc.rootfs = ${rootfs}

# userns 1-to-1 mapping
lxc.id_map = u 0 0 65536
lxc.id_map = g 0 0 65536

lxc.haltsignal = SIGTERM

lxc.pts = 256
lxc.tty = 0

lxc.cgroup.devices.deny = a

lxc.mount.auto = proc sys cgroup
lxc.mount.entry = /bin bin none ro,bind 0 0
lxc.mount.entry = /etc etc none ro,bind 0 0
lxc.mount.entry = /lib lib none ro,bind 0 0
lxc.mount.entry = /sbin sbin none ro,bind 0 0
lxc.mount.entry = /usr usr none ro,rbind 0 0
lxc.mount.entry = /tmp/ut-run1 var/run none rw,bind 0 0
EOF

if [ "$(uname -m)" = "x86_64" ]; then
cat <<EOF >> $path/config
lxc.mount.entry = /lib64 lib64 none ro,bind 0 0
EOF
fi


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

br_name="virbr-${name}"
sub_net="103" # TODO from param

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
lxc.mount.entry = /var/run/containers/${name}/run var/run none rw,bind 0 0

lxc.network.type = veth
lxc.network.link =  ${br_name}
lxc.network.flags = up
lxc.network.name = eth0
lxc.network.veth.pair = veth-${name}
lxc.network.ipv4.gateway = 10.0.${sub_net}.1
lxc.network.ipv4 = 10.0.${sub_net}.2/24

lxc.hook.pre-start = ${path}/pre-start.sh

#lxc.loglevel = TRACE
#lxc.logfile = /tmp/${name}.log
EOF

# prepare pre start hook
cat <<EOF >> ${path}/pre-start.sh
if [ -z "\$(/usr/sbin/brctl show | /bin/grep -P "${br_name}\t")" ]
then
    /usr/sbin/brctl addbr ${br_name}
    /usr/sbin/brctl setfd ${br_name} 0
    /sbin/ifconfig ${br_name} 10.0.${sub_net}.1 netmask 255.255.255.0 up
fi
if [ -z "\$(/usr/sbin/iptables -t nat -S | /bin/grep MASQUERADE)" ]
then
    /bin/echo 1 > /proc/sys/net/ipv4/ip_forward
    /usr/sbin/iptables -t nat -A POSTROUTING -s 10.0.0.0/16 ! -d 10.0.0.0/16 -j MASQUERADE
fi
EOF

chmod 755 ${path}/pre-start.sh

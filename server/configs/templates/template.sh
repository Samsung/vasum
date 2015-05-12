#!/bin/bash

echo LXC template, args: $@

options=$(getopt -o p:n: -l path:,rootfs:,name:,vt:,ipv4:,ipv4-gateway: -- "$@")
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
        --vt)           vt=$2; shift 2;;
        --ipv4)         ipv4=$2; shift 2;;
        --ipv4-gateway) ipv4_gateway=$2; shift 2;;
        --)             shift 1; break ;;
        *)              break ;;
    esac
done

br_name="virbr-${name}"

# XXX assume rootfs if mounted from iso

# Prepare zone configuration file
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

# create a separate network per zone
# - it forbids traffic sniffing (like macvlan in bridge mode)
# - it enables traffic controlling from host using iptables
lxc.network.type = veth
lxc.network.link =  ${br_name}
lxc.network.flags = up
lxc.network.name = eth0
lxc.network.veth.pair = veth-${name}
lxc.network.ipv4.gateway = ${ipv4_gateway}
lxc.network.ipv4 = ${ipv4}/24

lxc.hook.pre-start = ${path}/pre-start.sh

#lxc.loglevel = TRACE
#lxc.logfile = /tmp/${name}.log
EOF

# prepare pre start hook
> ${path}/pre-start.sh
cat <<EOF >> ${path}/pre-start.sh
if [ -z "\$(/usr/sbin/brctl show | /bin/grep -P "${br_name}\t")" ]
then
    /usr/sbin/brctl addbr ${br_name}
    /usr/sbin/brctl setfd ${br_name} 0
    /sbin/ifconfig ${br_name} ${ipv4_gateway} netmask 255.255.255.0 up
fi
if [ -z "\$(/usr/sbin/iptables -t nat -S | /bin/grep MASQUERADE)" ]
then
    /bin/echo 1 > /proc/sys/net/ipv4/ip_forward
    /usr/sbin/iptables -t nat -A POSTROUTING -s 10.0.0.0/16 ! -d 10.0.0.0/16 -j MASQUERADE
fi
EOF

chmod 755 ${path}/pre-start.sh

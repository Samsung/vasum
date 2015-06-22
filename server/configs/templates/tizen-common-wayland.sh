#!/bin/bash

#  lxc-tizen template script
#
#  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
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
Optional args:
  -p,--path         path to zone config files
  --rootfs          path to zone rootfs
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

br_name="virbr-${name}"
broadcast="$(echo $ipv4|cut -d . -f -3).255"

# Prepare zone rootfs
ROOTFS_DIRS="\
${rootfs}/bin \
${rootfs}/dev \
${rootfs}/etc \
${rootfs}/home \
${rootfs}/home/alice \
${rootfs}/home/bob \
${rootfs}/home/carol \
${rootfs}/home/developer \
${rootfs}/home/guest \
${rootfs}/lib \
${rootfs}/media \
${rootfs}/mnt \
${rootfs}/opt \
${rootfs}/proc \
${rootfs}/root \
${rootfs}/run \
${rootfs}/sbin \
${rootfs}/sys \
${rootfs}/tmp \
${rootfs}/usr \
${rootfs}/var \
${rootfs}/var/run \
${path}/hooks \
${path}/scripts \
${path}/systemd \
${path}/systemd/system \
${path}/systemd/system/multi-user.target.wants \
${path}/systemd/user
"
/bin/mkdir ${ROOTFS_DIRS}
/bin/chown alice:users ${rootfs}/home/alice
/bin/chown bob:users ${rootfs}/home/bob
/bin/chown carol:users ${rootfs}/home/carol
/bin/chown developer:users ${rootfs}/home/developer
/bin/chown guest:users ${rootfs}/home/guest

/bin/ln -s /dev/null ${path}/systemd/system/bluetooth.service
/bin/ln -s /dev/null ${path}/systemd/system/sshd.service
/bin/ln -s /dev/null ${path}/systemd/system/sshd.socket
/bin/ln -s /dev/null ${path}/systemd/system/sshd@.service
/bin/ln -s /dev/null ${path}/systemd/system/systemd-udevd.service
/bin/ln -s /dev/null ${path}/systemd/system/systemd-udevd-kernel.socket
/bin/ln -s /dev/null ${path}/systemd/system/systemd-udevd-control.socket
/bin/ln -s /dev/null ${path}/systemd/system/vasum.service
/bin/ln -s /dev/null ${path}/systemd/system/vasum.socket
/bin/ln -s /dev/null ${path}/systemd/system/vconf-setup.service
/bin/ln -s /usr/lib/systemd/system/tlm.service ${path}/systemd/system/multi-user.target.wants/tlm.service
/bin/ln -s /dev/null ${path}/systemd/user/media-server-user.service

cat <<EOF >>${path}/systemd/system/display-manager-run.service
# Run weston with framebuffer backend on selected virtual terminal.
[Unit]
Description=Weston display daemon

[Service]
User=display
WorkingDirectory=/run/%u
# FIXME: log files shouldn't be stored in tmpfs directories (can get quite big and have side effects)
#ExecStart=/bin/sh -c 'backend=drm ; [ -d /dev/dri ] || backend=fbdev ; exec /usr/bin/weston --backend=$backend-backend.so -i0 --log=/run/%u/weston.log'
ExecStart=/usr/bin/weston --backend=fbdev-backend.so -i0 --log=/tmp/weston.log --tty=${vt}
#StandardInput=tty
#TTYPath=/dev/tty7
EnvironmentFile=/etc/sysconfig/weston
Restart=on-failure
RestartSec=10

#adding the capability to configure ttys
#may be needed if the user 'display' doesn't own the tty
CapabilityBoundingSet=CAP_SYS_TTY_CONFIG

[Install]
WantedBy=graphical.target
EOF
chmod 644 ${path}/systemd/system/display-manager-run.service

# TODO temporary solution to set proper access rights
cat <<EOF >>${path}/systemd/system/ptmx-fix.service
[Unit]
Description=Temporary fix access rights

[Service]
ExecStart=/bin/sh -c '/bin/chmod 666 /dev/ptmx; /bin/chown root:tty /dev/ptmx; /bin/chsmack -a "*" /dev/ptmx'

[Install]
WantedBy=multi-user.target
EOF
chmod 644 ${path}/systemd/system/ptmx-fix.service
/bin/ln -s ${path}/systemd/system/ptmx-fix.service ${path}/systemd/system/multi-user.target.wants/ptmx-fix.service

sed -e 's/run\/display/tmp/g' /usr/lib/systemd/system/display-manager.path >> ${path}/systemd/system/display-manager.path
chmod 644 ${path}/systemd/system/display-manager.path
sed -e 's/run\/display/tmp/g' /usr/lib/systemd/system/display-manager.service >> ${path}/systemd/system/display-manager.service
chmod 644 ${path}/systemd/system/display-manager.service
sed -e 's/run\/display/tmp/g' /etc/sysconfig/weston >> ${path}/weston
sed -e 's/backgrounds\/tizen\/current/backgrounds\/tizen\/golfe-morbihan.jpg/g' /etc/xdg/weston/weston.ini >> ${path}/weston.ini
sed -e 's/run\/display/tmp/g' /usr/lib/systemd/user/weston-user.service >> ${path}/systemd/user/weston-user.service
sed -e 's/run\/display/tmp/g' /etc/tlm.conf >> ${path}/tlm.conf
sed -e 's/run\/display/tmp/g' /etc/session.d/user-session >> ${path}/user-session
chmod 755 ${path}/user-session

# Prepare host configuration
cat <<EOF >>/etc/udev/rules.d/99-tty.rules
SUBSYSTEM=="tty", KERNEL=="tty${vt}", OWNER="display", SECLABEL{smack}="^"
EOF

cat <<EOF >/etc/systemd/system/display-manager-run.service
# Run weston with framebuffer backend on tty7.
[Unit]
Description=Weston display daemon

[Service]
User=display
WorkingDirectory=/run/%u
# FIXME: log files shouldn't be stored in tmpfs directories (can get quite big and have side effects)
#ExecStart=/bin/sh -c 'backend=drm ; [ -d /dev/dri ] || backend=fbdev ; exec /usr/bin/weston --backend=$backend-backend.so -i0 --log=/run/%u/weston.log'
ExecStart=/usr/bin/weston --backend=fbdev-backend.so -i0 --log=/run/%u/weston.log
StandardInput=tty
TTYPath=/dev/tty7
EnvironmentFile=/etc/sysconfig/weston
Restart=on-failure
RestartSec=10

#adding the capability to configure ttys
#may be needed if the user 'display' doesn't own the tty
#CapabilityBoundingSet=CAP_SYS_TTY_CONFIG

[Install]
WantedBy=graphical.target
EOF

# Prepare zone configuration file
cat <<EOF >>${path}/config
lxc.utsname = ${name}
lxc.rootfs = ${rootfs}

#lxc.cap.drop = audit_control
#lxc.cap.drop = audit_write
#lxc.cap.drop = mac_admin
#lxc.cap.drop = mac_override
#lxc.cap.drop = mknod
#lxc.cap.drop = setfcap
#lxc.cap.drop = setpcap
#lxc.cap.drop = sys_admin
#lxc.cap.drop = sys_boot
#lxc.cap.drop = sys_chroot #required by SSH
#lxc.cap.drop = sys_module
#lxc.cap.drop = sys_nice
#lxc.cap.drop = sys_pacct
#lxc.cap.drop = sys_rawio
#lxc.cap.drop = sys_resource
#lxc.cap.drop = sys_time
#lxc.cap.drop = sys_tty_config #required by getty

lxc.cgroup.devices.deny = a
lxc.cgroup.devices.allow = c 1:* rwm #/dev/null, /dev/zero, ...
lxc.cgroup.devices.allow = c 5:* rwm #/dev/console, /dev/ptmx, ...
lxc.cgroup.devices.allow = c 136:* rwm #/dev/pts/0 ...
lxc.cgroup.devices.allow = c 10:223 rwm #/dev/uinput
lxc.cgroup.devices.allow = c 13:64 rwm #/dev/input/event0
lxc.cgroup.devices.allow = c 13:65 rwm #/dev/input/event1
lxc.cgroup.devices.allow = c 13:66 rwm #/dev/input/event2
lxc.cgroup.devices.allow = c 13:67 rwm #/dev/input/event3
lxc.cgroup.devices.allow = c 13:68 rwm #/dev/input/event4
lxc.cgroup.devices.allow = c 13:69 rwm #/dev/input/event5
lxc.cgroup.devices.allow = c 13:63 rwm #/dev/input/mice
lxc.cgroup.devices.allow = c 13:32 rwm #/dev/input/mouse0
lxc.cgroup.devices.allow = c 226:0 rwm #/dev/dri/card0
lxc.cgroup.devices.allow = c 2:* rwm #/dev/pty

lxc.pts = 256
lxc.tty = 0
#lxc.console=/dev/tty1
lxc.kmsg = 0

#lxc.cgroup.cpu.shares = 1024
#lxc.cgroup.cpuset.cpus = 0,1,2,3
#lxc.cgroup.memory.limit_in_bytes       = 512M
#lxc.cgroup.memory.memsw.limit_in_bytes = 1G
#lxc.cgroup.blkio.weight                = 500

lxc.mount.auto = proc sys:rw cgroup
lxc.mount = ${path}/fstab

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

lxc.hook.pre-start = ${path}/hooks/pre-start.sh
#lxc.hook.post-stop = ${path}/hooks/post-stop.sh
EOF

# Prepare zone hook files
cat <<EOF >>${path}/hooks/pre-start.sh
if ! /usr/sbin/ip link show ${br_name} &>/dev/null
then
    /usr/sbin/ip link add name ${br_name} type bridge
    /usr/sbin/ip link set ${br_name} up
    /usr/sbin/ip addr add ${ipv4}/24 broadcast ${broadcast} dev ${br_name}
fi
if [ -z "\$(/usr/sbin/iptables -t nat -S | /bin/grep MASQUERADE)" ]
then
    /bin/echo 1 > /proc/sys/net/ipv4/ip_forward
    /usr/sbin/iptables -t nat -A POSTROUTING -s 10.0.0.0/16 ! -d 10.0.0.0/16 -j MASQUERADE
fi
EOF

chmod 770 ${path}/hooks/pre-start.sh

# Prepare zone fstab file
cat <<EOF >>${path}/fstab
/bin bin none ro,bind 0 0
/etc etc none ro,bind 0 0
${path}/tlm.conf etc/tlm.conf none ro,bind 0 0
${path}/user-session etc/session.d/user-session none rw,bind 0 0
${path}/systemd/system etc/systemd/system none ro,bind 0 0
${path}/systemd/user etc/systemd/user none ro,bind 0 0
${path}/weston etc/sysconfig/weston none ro,bind 0 0
${path}/weston.ini etc/xdg/weston/weston.ini none ro,bind 0 0
/lib lib none ro,bind 0 0
/media media none ro,bind 0 0
/mnt mnt none ro,bind 0 0
/sbin sbin none ro,bind 0 0
/usr usr none ro,rbind 0 0
/opt opt none rw,rbind 0 0
devtmpfs dev devtmpfs rw,relatime,mode=755 0 0
devpts dev/pts devpts rw,relatime,gid=5,mode=620,ptmxmode=000 0 0
/sys/fs/smackfs sys/fs/smackfs none rw,bind 0 0
#/var/run/zones/${name}/run var/run none rw,bind 0 0
#tmpfs run tmpfs rw,nosuid,nodev,mode=755 0 0
EOF

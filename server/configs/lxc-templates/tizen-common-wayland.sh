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
    $1 -n|--name=<container_name>
        [-p|--path=<path>] [-r|--rootfs=<rootfs>] [-v|--vt=<vt>]
        [--ipv4=<ipv4>] [--ipv4-gateway=<ipv4_gateway>] [-h|--help]
Mandatory args:
  -n,--name         container name
Optional args:
  -p,--path         path to container config files, defaults to /var/lib/lxc
  --rootfs          path to container rootfs, defaults to /var/lib/lxc/[NAME]/rootfs
  -v,--vt           container virtual terminal
  --ipv4            container IP address
  --ipv4-gateway    container gateway
  -h,--help         print help
EOF
    return 0
}

options=$(getopt -o hp:v:n: -l help,rootfs:,path:,vt:,name:,ipv4:,ipv4-gateway: -- "$@")
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
        -v|--vt)        vt=$2; shift 2;;
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
    echo "Container name must be given"
    exit 1
fi

if [ -z "$path" ]; then
    echo "'path' parameter is required"
    exit 1
fi

br_name="virbr-${name}"

# Prepare container rootfs
ROOTFS_DIRS="\
${rootfs}/bin \
${rootfs}/dev \
${rootfs}/etc \
${rootfs}/home \
${rootfs}/home/alice \
${rootfs}/home/bob \
${rootfs}/home/carol \
${rootfs}/home/guest \
${rootfs}/lib \
${rootfs}/media \
${rootfs}/mnt \
${rootfs}/opt \
${rootfs}/proc \
${rootfs}/root \
${rootfs}/run \
${rootfs}/run/dbus \
${rootfs}/run/memory \
${rootfs}/run/systemd \
${rootfs}/run/udev \
${rootfs}/sbin \
${rootfs}/sys \
${rootfs}/tmp \
${rootfs}/usr \
${path}/hooks \
${path}/scripts \
${path}/systemd \
${path}/systemd/system \
${path}/systemd/user
"
/bin/mkdir ${ROOTFS_DIRS}
/bin/chown alice:users ${rootfs}/home/alice
/bin/chown bob:users ${rootfs}/home/bob
/bin/chown carol:users ${rootfs}/home/carol
/bin/chown guest:users ${rootfs}/home/guest

/bin/ln -s /dev/null ${path}/systemd/system/bluetooth.service
/bin/ln -s /dev/null ${path}/systemd/system/sshd.service
/bin/ln -s /dev/null ${path}/systemd/system/sshd.socket
/bin/ln -s /dev/null ${path}/systemd/system/sshd@.service
/bin/ln -s /dev/null ${path}/systemd/system/systemd-udevd.service
/bin/ln -s /dev/null ${path}/systemd/system/user-session-launch@seat0-5100.service
/bin/ln -s /dev/null ${path}/systemd/system/vconf-setup.service

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
EnvironmentFile=/etc/systemd/system/weston
Restart=on-failure
RestartSec=10

#adding the capability to configure ttys
#may be needed if the user 'display' doesn't own the tty
CapabilityBoundingSet=CAP_SYS_TTY_CONFIG

[Install]
WantedBy=graphical.target
EOF

cat <<EOF >>${path}/systemd/system/display-manager.path
# Wayland socket path is changed to /tmp directory.
[Unit]
Description=Wait for wayland socket
Requires=display-manager-run.service
After=display-manager-run.service

[Path]
PathExists=/tmp/wayland-0
EOF

cat <<EOF >>${path}/systemd/system/display-manager.service
# Wayland socket path is changed to /tmp directory.
[Unit]
Description=Display manager setup service
Requires=display-manager-run.service
After=display-manager-run.service

[Service]
Type=oneshot
ExecStart=/usr/bin/chmod g+w /tmp/wayland-0
#ExecStart=/usr/bin/chsmack -a User /tmp/wayland-0

[Install]
WantedBy=graphical.target
EOF

cat <<EOF >>${path}/systemd/system/weston
# path to display manager runtime dir
XDG_RUNTIME_DIR=/tmp
XDG_CONFIG_HOME=/etc/systemd/system
EOF

cat <<EOF >>${path}/systemd/system/weston.ini
# Weston config for container.
[core]
modules=desktop-shell.so

[shell]
background-image=/usr/share/backgrounds/tizen/golfe-morbihan.jpg
background-color=0xff002244
background-type=scale-crop
panel-color=0x95333333
locking=true
panel=false
animation=zoom
#binding-modifier=ctrl
num-workspaces=4
#cursor-theme=whiteglass
#cursor-size=24
startup-animation=fade

#lockscreen-icon=/usr/share/icons/gnome/256x256/actions/lock.png
#lockscreen=/usr/share/backgrounds/gnome/Garden.jpg
#homescreen=/usr/share/backgrounds/gnome/Blinds.jpg

## weston

[launcher]
icon=/usr/share/icons/tizen/32x32/terminal.png
path=/usr/bin/weston-terminal

[screensaver]
# Uncomment path to disable screensaver
duration=600

[input-method]
path=/usr/libexec/weston-keyboard
#path=/bin/weekeyboard

#[keyboard]
#keymap_layout=fr

#[output]
#name=LVDS1
#mode=1680x1050
#transform=90
#icc_profile=/usr/share/color/icc/colord/Bluish.icc

#[output]
#name=VGA1
#mode=173.00  1920 2048 2248 2576  1080 1083 1088 1120 -hsync +vsync
#transform=flipped

#[output]
#name=X1
#mode=1024x768
#transform=flipped-270

#[touchpad]
#constant_accel_factor = 50
#min_accel_factor = 0.16
#max_accel_factor = 1.0

[output]
name=DP1
default_output=1
EOF

cat <<EOF >>${path}/systemd/user/weston-user.service
# Wayland socket path is changed to /tmp directory.
[Unit]
Description=Shared weston session

[Service]
ExecStartPre=/usr/bin/ln -sf /tmp/wayland-0 /run/user/%U/
ExecStart=/bin/sh -l -c "/usr/bin/tz-launcher -c /usr/share/applications/tizen/launcher.conf %h/.applications/desktop"
EnvironmentFile=/etc/sysconfig/weston-user

[Install]
WantedBy=default.target
EOF

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

# Prepare container configuration file
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

# create a separate network per container
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

# Prepare container hook files
cat <<EOF >>${path}/hooks/pre-start.sh
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

chmod 770 ${path}/hooks/pre-start.sh

# Prepare container fstab file
cat <<EOF >>${path}/fstab
/bin bin none ro,bind 0 0
/etc etc none ro,bind 0 0
${path}/systemd/system etc/systemd/system none ro,bind 0 0
${path}/systemd/user etc/systemd/user none ro,bind 0 0
/lib lib none ro,bind 0 0
/media media none ro,bind 0 0
/mnt mnt none ro,bind 0 0
/sbin sbin none ro,bind 0 0
/usr usr none ro,rbind 0 0
/opt opt none rw,rbind 0 0
devtmpfs dev devtmpfs rw,relatime,mode=755 0 0
devpts dev/pts devpts rw,relatime,gid=5,mode=620,ptmxmode=000 0 0
/run/dbus run/dbus none rw,bind 0 0
/run/memory run/memory none rw,bind 0 0
/run/systemd run/systemd none rw,bind 0 0
/run/udev run/udev none rw,bind 0 0
/sys/fs/smackfs sys/fs/smackfs none rw,bind 0 0
#tmpfs run tmpfs rw,nosuid,nodev,mode=755 0 0
EOF

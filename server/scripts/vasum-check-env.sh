#!/bin/sh

# Note: based on lxc-checkconfig script

: ${CONFIG:=/proc/config.gz}
: ${GREP:=zgrep}

SETCOLOR_SUCCESS="printf \\033[1;32m"
SETCOLOR_FAILURE="printf \\033[1;31m"
SETCOLOR_WARNING="printf \\033[1;33m"
SETCOLOR_NORMAL="printf \\033[0;39m"

FAILURE=false

is_builtin() {
    $GREP -q "$1=y" $CONFIG
    return $?
}

# is_enabled has two parameters
# 1=kernel option
# 2=is the option mandatory (optional parameter)
is_enabled() {
    mandatory=$2
    is_builtin $1
    RES=$?
    if [ $RES -eq 0 ]; then
        $SETCOLOR_SUCCESS && echo "enabled" && $SETCOLOR_NORMAL
    else
        if [ ! -z "$mandatory" -a "$mandatory" = yes ]; then
            FAILURE=true
            $SETCOLOR_FAILURE && echo "required" && $SETCOLOR_NORMAL
        else
            $SETCOLOR_WARNING && echo "missing" && $SETCOLOR_NORMAL
        fi
    fi
}


if [ ! -f $CONFIG ]; then
    echo "no $CONFIG, searching..."
    CONFIG="/boot/config-$(uname -r)"
    if [ ! -f $CONFIG ]; then
        echo "kernel config not found"; exit 1;
    fi
fi

echo "kernel config: $CONFIG"
KVER=$($GREP '^# Linux.*Kernel Configuration' $CONFIG | sed -r 's/#.* ([0-9]+\.[0-9]+\.[0-9]+).*/\1/')

KVER_MAJOR=$(echo $KVER | sed -r 's/([0-9])\.[0-9]{1,2}\.[0-9]{1,3}.*/\1/')
if [ "$KVER_MAJOR" = "2" ]; then
    KVER_MINOR=$(echo $KVER | sed -r 's/2.6.([0-9]{2}).*/\1/')
else
    KVER_MINOR=$(echo $KVER | sed -r 's/[0-9]\.([0-9]{1,3})\.[0-9]{1,3}.*/\1/')
fi

echo "kver=$KVER, major.minor=$KVER_MAJOR.$KVER_MINOR"

echo "--- Namespaces ---"
echo -n "Namespaces: " && is_enabled CONFIG_NAMESPACES yes
echo -n "Utsname namespace: " && is_enabled CONFIG_UTS_NS yes
echo -n "Ipc namespace: " && is_enabled CONFIG_IPC_NS yes
echo -n "Pid namespace: " && is_enabled CONFIG_PID_NS yes
echo -n "User namespace: " && is_enabled CONFIG_USER_NS yes
echo -n "Network namespace: " && is_enabled CONFIG_NET_NS yes
echo -n "Multiple /dev/pts instances: " && is_enabled DEVPTS_MULTIPLE_INSTANCES yes
echo
echo "--- Network virtualization ---"
echo -n "VETH: " && is_enabled CONFIG_VETH yes
echo -n "VLAN: " && is_enabled CONFIG_VLAN_8021Q yes
echo -n "MACVLAN: " && is_enabled CONFIG_MACVLAN yes
echo
echo "--- Control groups ---"
echo -n "Cgroup: " && is_enabled CONFIG_CGROUPS yes
echo -n "PID cpuset: " && is_enabled CONFIG_PROC_PID_CPUSET yes
echo
echo -n "CFS Bandwidth: " && is_enabled CONFIG_CFS_BANDWIDTH yes

if $FAILURE; then
    exit 1
else
    exit 0
fi

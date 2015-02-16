#Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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
# @file   network_common.py.in
# @author Jacek Pielaszkiewicz (j.pielaszkie@samsung.com)
#

from vsm_integration_tests.common import vsm_test_utils
import subprocess
import string
import sys
import os
import traceback

# Debug command on/off
DEBUG_COMMAND=False

# Test urls
TEST_URL_INTERNET=["www.samsung.com", "www.google.com", "www.oracle.com"]

#TODO read path from config (daemon.conf)
# Path to test zone
TEST_ZONE_PATH="/usr/share/.zones"

# Device Ethernet device
ETHERNET_DEVICE="usb0"
ETHERNET_DEVICE_DETECT=False

# Test zones
TEST_ZONE="test"
TEST_ZONE_ROOTFS=TEST_ZONE_PATH+"/"+TEST_ZONE
ZONES=[ TEST_ZONE ]

# Null device
OUTPUT_TO_NULL_DEVICE=" >/dev/null 2>&1 "

# Ping timeout
PING_TIME_OUT=3

# The class store test cases results
class TestInfo:
    testName = ""
    testItems = []

    def __init__(self, tn):
        self.testName = tn

class TestItem:
    itype = ""
    name = ""
    description = ""
    status = 0
    result = ""

    def __init__(self, tn, n):
        self.itype = tn
        self.name = n

# ----------------------------------------------------------
# Functions print info/error/warning message
#
def LOG_INFO(arg):
    print("[Info] " + arg)

def LOG_ERROR(arg):
    print("[Error] " + arg)

def LOG_WARNING(arg):
    print("[Warning] " + arg)

def LOG_DEBUG(arg):
    print("[Debug] " + arg)

# ----------------------------------------------------------
# The function tests mandatory user privileges
#
def test_run_user():
    if(os.getegid() != 0 or os.geteuid() != 0):
        return 1
    return 0

# ----------------------------------------------------------
# The function runs os command
#
def runCommand(cmd, blockDebug=False):
    null_device_str = OUTPUT_TO_NULL_DEVICE
    if(DEBUG_COMMAND):
        null_device_str = ""

    run_cmd = "( " + cmd + " ) " + null_device_str

    rc=0
    try:
        out=vsm_test_utils.launchProc(run_cmd)
        rc=out[0]
    except Exception:
        traceback.print_exc()
        rc=1

    if(DEBUG_COMMAND and not blockDebug):
        LOG_DEBUG("[DEBUG CMD] RC = " + str(rc) + "; CMD = " + run_cmd)

    return rc

# ----------------------------------------------------------
# The function runs os command and read output
#
def runCommandAndReadOutput(cmd):
    proc=subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    while(True):
        # Return code
        rcode=proc.poll()

        # Get line
        ret=proc.stdout.readline()
        ret=ret.translate(None, "\n")

        # Ignore empty lines
        if(ret != ""):
            yield ret

        # Test return code
        if(rcode is not None):
            break

# ----------------------------------------------------------
# The function checks whether zone path is present in system
#
def test_zone_path():
    rc = runCommand("ls " + TEST_ZONE_PATH)
    if( rc != 0 ):
        return 1
    return 0

# ----------------------------------------------------------
# The functions gets active ethernet device
#
def getActiveEthernetDevice():
    cmd=["/usr/sbin/ip -o link | /usr/bin/awk \' /ether/ { split( $2, list, \":\" ); print list[1] }\'"]
    iter = runCommandAndReadOutput(cmd)
    for val in iter:
        ETHERNET_DEVICE=val

    if(ETHERNET_DEVICE == ""):
        return 1

    return 0


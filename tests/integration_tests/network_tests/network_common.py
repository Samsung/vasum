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

from sc_integration_tests.common import sc_test_utils
import subprocess
import string
import sys
import os

# Debug command on/off
DEBUG_COMMAND=False

# Test urls
TEST_URL_INTERNET=["www.samsung.com", "www.google.com", "www.oracle.com"]

# Path to test container
TEST_CONTAINER_PATH="/opt/usr/containers/private"

# Device Ethernet device
ETHERNET_DEVICE="usb0"
ETHERNET_DEVICE_DETECT=False

# Test containers
CONTAINER_T1="business"
CONTAINER_T2="private"

containers=[CONTAINER_T1, CONTAINER_T2]

# Null device
OUTPUT_TO_NULL_DEVICE=" >/dev/null 2>&1 "

# Ping timeout
PING_TIME_OUT=3

# The calss store test cases results
class TestNetworkInfo:
    testName = ""
    testItemType = []
    testItemName = []
    testItemStatus = []
    testItemResult = []
    testItemDescription = []

    def __init__(self, tn):
        self.testName = tn

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
        out=sc_test_utils.launchProc(run_cmd)
    except Exception:
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
# The function checks whether test container image is present in system
#
def test_guest_image():
    rc = runCommand("/usr/bin/chroot " + TEST_CONTAINER_PATH + " /bin/true")
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

# ----------------------------------------------------------
# The function checks whether mandatory tools are present in
# the system
#
def test_mandatory_toos():

    tools     =["/usr/bin/ping"]
    root_tools=[TEST_CONTAINER_PATH]

    for i in range(len(tools)):
        rc = runCommand("/usr/bin/ls " + root_tools[i] + tools[i])
        if( rc != 0 ):
            if( root_tools[i] != "" ):
                LOG_ERROR("No " + tools[i] + " command in guest")
            else:
                LOG_ERROR("No " + tools[i] + " command in host")
            return 1
    return 0

def virshCmd(args):
    return runCommand("/usr/bin/virsh -c lxc:/// " + args)

# ----------------------------------------------------------
# The function tests single test case result
#
def test_result(expected_result, result):
    if((expected_result >= 0 and result == expected_result) or (expected_result < 0 and result != 0)):
        return 0
    return 1

# ----------------------------------------------------------
# The function performs single internet access test
#
def internetAccessTest(container):
    count=0
    for item in TEST_URL_INTERNET:
        LOG_INFO("           Test for URL : " + item);
        rc = virshCmd("lxc-enter-namespace " + container + \
                    " --noseclabel -- /usr/bin/ping -c 3 -W " + \
                    str(PING_TIME_OUT) + " " + item)
        if(rc != 0):
            count = count + 1

    if(count != 0):
        return 1

    return 0;

# ----------------------------------------------------------
# The function performs single internet access test
#
def networkVisibiltyTest(container, dest_ip):
    return virshCmd("lxc-enter-namespace " + container + \
                    " --noseclabel -- /usr/bin/ping -c 3 -W " + \
                    str(PING_TIME_OUT) + " " + dest_ip)

def printInternetAccessTestStatus(container, testInfo1):

    text = "          Internet access for container: " + container + \
           "; TCS = " + testInfo1.testItemResult[len(testInfo1.testItemResult)-1]

    if(testInfo1.testItemResult[len(testInfo1.testItemResult)-1] == "Success"):
        LOG_INFO(text)
    else:
        LOG_ERROR(text)

def networkVisibiltyTestStatus(src, dest, ip, testInfo2):

    text = "          Container access: " + src + \
          " -> " + dest + \
          " [" + ip + "]" + \
          "; TCS = " + testInfo2.testItemResult[len(testInfo2.testItemResult)-1]

    if(testInfo2.testItemResult[len(testInfo2.testItemResult)-1] == "Success"):
        LOG_INFO(text)
    else:
        LOG_ERROR(text)

# ----------------------------------------------------------
# The function performs test case for two containers - Business and Private.
# Both containers are mutually isolated and have access to the Internet.
#
def twoNetworks():
    ltestInfo = TestNetworkInfo("Two networks tests")

    # 0. Test data
    containers_list      = [CONTAINER_T1, CONTAINER_T2]
    dest_containers_list = [CONTAINER_T2, CONTAINER_T1]
    test_ip_list         = [["192.168.101.2"], ["192.168.102.2"]]
    test_1_expected_res  = [ 0,  0]
    test_2_expected_res  = [-1, -1]

    # 1. Enable internet access for both networks
    LOG_INFO("   - Setup device")

    # 2. Internet access
    LOG_INFO("   - Two containers environment network test case execution")
    LOG_INFO("     - Internet access test")
    for i in range(len(containers_list)):

        # - Test case info
        ltestInfo.testItemType.append("[Two nets] Internet access")
        ltestInfo.testItemName.append(containers_list[i])
        ltestInfo.testItemDescription.append("Internet access test for : " + containers_list[i])

        # - Perform test
        rc = internetAccessTest(containers_list[i])

        # - Test status store
        if(test_result(test_1_expected_res[i], rc) == 0):
            ltestInfo.testItemStatus.append(0)
            ltestInfo.testItemResult.append("Success")
        else:
            ltestInfo.testItemStatus.append(1)
            ltestInfo.testItemResult.append("Error")

        # - Print status
        printInternetAccessTestStatus(containers_list[i], ltestInfo)

    # 3. Mutual containers visibility
    LOG_INFO("     - Containers isolation")
    for i in range(len(containers_list)):
        # Interate over destynation ips
        dest_ips = test_ip_list[i]

        for j in range(len(dest_ips)):
            # - Test case info
            ltestInfo.testItemType.append("[Two nets] Visibility")
            ltestInfo.testItemName.append(containers_list[i] + "->" + dest_containers_list[i])
            ltestInfo.testItemDescription.append("Container access for : " + containers_list[i])

            # Perform test
            rc = networkVisibiltyTest(containers_list[i], dest_ips[j])

            # - Test status store
            if(test_result(test_2_expected_res[i], rc) == 0):
                ltestInfo.testItemStatus.append(0)
                ltestInfo.testItemResult.append("Success")
            else:
                ltestInfo.testItemStatus.append(1)
                ltestInfo.testItemResult.append("Error")

            # - Print status
            networkVisibiltyTestStatus(containers_list[i], dest_containers_list[i], dest_ips[j], ltestInfo)

    LOG_INFO("   - Clean environment")

    return ltestInfo

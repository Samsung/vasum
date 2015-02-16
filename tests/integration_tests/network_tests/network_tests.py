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
# @file   network_tests.py
# @author Jacek Pielaszkiewicz (j.pielaszkie@samsung.com)
#

'''! Module used to test network in zones

@author: Jacek Pielaszkiewicz (j.pielaszkie@samsung.com)
'''
import unittest
from vsm_integration_tests.common import vsm_test_utils
from network_common import *

class NetworkTestCase(unittest.TestCase):
    '''! Test case to check network configuration
    '''
    def setUp(self):
        # Function setup host machine to perform tests
        #
        # 1. Check user permisions
        if(test_run_user() == 1):
            self.assertTrue(False, "ROOT user is required to run the test")
            return

        # 2. Test zone path
        if(test_zone_path() == 1):
            self.assertTrue(False, "No test zone path :" + TEST_ZONE_PATH)
            return

        # 3. Ethernet device obtaning
        if(ETHERNET_DEVICE_DETECT and getActiveEthernetDevice() == 1):
            self.assertTrue(False, "Cannot obtain ethernet device")
            return

    def test_01twoNetworks(self):
        '''! Checks networks configuration
        '''

def main():
    unittest.main(verbosity=2)

if __name__ == "__main__":
    main()

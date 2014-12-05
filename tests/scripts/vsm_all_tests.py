#!/usr/bin/env python

import vsm_launch_test
import sys

# insert other test binaries to this array
_testCmdTable = ["vasum-server-unit-tests"]

for test in _testCmdTable:
    vsm_launch_test.launchTest([test] + sys.argv[1:])

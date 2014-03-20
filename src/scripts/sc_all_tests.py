#!/usr/bin/env python

import sc_launch_test
import sys

# insert other test binaries to this array
_testCmdTable = ["security-containers-server-unit-tests"]

for test in _testCmdTable:
    sc_launch_test.launchTest([test] + sys.argv[1:])

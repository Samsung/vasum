#!/usr/bin/env python

import sc_test_launch

# insert other test binaries to this array
_testCmdTable = ["security-containers-server-unit-tests"]

for test in _testCmdTable:
    sc_test_launch.launchTest([test])

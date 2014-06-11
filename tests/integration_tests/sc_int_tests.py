#!/usr/bin/env python
'''@package: sc_integration_tests
@author: Lukasz Kostyra (l.kostyra@samsung.com)

Security-containers integration tests launcher. Launches all integration tests.
'''
import unittest

from sc_integration_tests.image_tests import *

# add tests here...
test_groups = [
               image_tests
              ]


def main():
    for test_group in test_groups:
        print "Starting", test_group.__name__, " ..."
        suite = unittest.TestLoader().loadTestsFromModule(test_group)
        unittest.TextTestRunner(verbosity=2).run(suite)
        print "\n"

if __name__ == "__main__":
    main()

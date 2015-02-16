'''! Module used to collect list of zones based on the vasum configuration files.

@author: Michal Witanowski (m.witanowski@samsung.com)
'''
import os
import json
import xml.etree.ElementTree as ET
from vsm_integration_tests.common import vsm_test_utils
from pprint import pprint


class ConfigChecker:
    '''! This class verifies vasum configuration files and collects dictionary with
         zones existing in the system (name and rootfs path).
    '''

    def __init__(self, confpath):
        print "ConfigChecker", confpath
        with open(confpath) as jf:
            self.conf = json.load(jf)

       # TODO: extract data for tests from json object

        self.zones = {}
    #TODO prepate zones config into list of (zone_name,zone_roofs_path)
    #       self.conf["zoneConfigs"] -> (zone, self.conf["zonesPath"]+"/"+zone)

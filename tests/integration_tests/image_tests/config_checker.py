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

    def __parseLibvirtXML(self, path):
        '''! Parses libvirt's configuration in order to extract zone name and path.

         @param path    Libvirt's zone configuration path
        '''
        tree = ET.parse(path)
        root = tree.getroot()
        name = root.find("name").text
        rootFound = False

        # extract directory mountings
        for elem in root.iterfind('devices/filesystem'):
            if "type" not in elem.attrib:
                raise Exception("'type' attribute not found for 'filesystem' node in file: " + path)

            nodeSource = elem.find("source")
            if nodeSource is None:
                raise Exception("'source' not found in 'filesystem' node in file: " + path)

            nodeTarget = elem.find("target")
            if nodeTarget is None:
                raise Exception("'target' not found in 'filesystem' node in file: " + path)

            source = nodeSource.attrib["dir"]
            target = nodeTarget.attrib["dir"]
            if target == "/":
                if rootFound:
                    raise Exception("Multiple root fs mounts found in file: " + path)
                else:
                    self.zones[name] = source
                    print "    Zone '" + name + "' found at: " + source
                    rootFound = True

        if not rootFound:
            raise Exception("Root directory of '" + name + "' zone not specified in XML")

    def __init__(self, mainConfigPath):
        '''! Parses daemon's JSON configuration files.

         @param mainConfigPath    Path to the main config "daemon.conf"
        '''
        self.zones = {}
        print "Looking for zone IDs..."

        # load main daemon JSON config file
        if not os.path.isfile(mainConfigPath):
            raise Exception(mainConfigPath + " not found. " +
                            "Please verify that vasum is properly installed.")
        with open(mainConfigPath) as daemonConfigStr:
            daemonConfigData = json.load(daemonConfigStr)
            daemonConfigDir = os.path.dirname(os.path.abspath(mainConfigPath))

            # get dictionary with zones
            zoneConfigPaths = daemonConfigData["zoneConfigs"]
            for configPath in zoneConfigPaths:

                # open zone config file
                zoneConfigPath = os.path.join(daemonConfigDir, configPath)
                if not os.path.isfile(zoneConfigPath):
                    raise Exception(zoneConfigPath + " not found. " +
                                    "Please verify that vasum is properly installed.")
                with open(zoneConfigPath) as zoneConfigStr:
                    zoneConfigData = json.load(zoneConfigStr)

                    # extract XML config path for libvirt
                    libvirtConfigPath = os.path.join(daemonConfigDir, "zones",
                                                     zoneConfigData["config"])

                    output, ret = vsm_test_utils.launchProc("virt-xml-validate " + libvirtConfigPath)
                    if ret == 0:
                        self.__parseLibvirtXML(libvirtConfigPath)
                    else:
                        raise Exception(output)

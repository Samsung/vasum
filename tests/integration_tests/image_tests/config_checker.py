'''! Module used to collect list of containers based on the security-containers configuration files.

@author: Michal Witanowski (m.witanowski@samsung.com)
'''
import os
import json
import xml.etree.ElementTree as ET
from sc_integration_tests.common import sc_test_utils
from pprint import pprint


class ConfigChecker:
    '''! This class verifies security-containers configuration files and collects dictionary with
         containers existing in the system (name and rootfs path).
    '''

    def __parseLibvirtXML(self, path):
        '''! Parses libvirt's configuration in order to extract container name and path.

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
                    self.containers[name] = source
                    print "    Container '" + name + "' found at: " + source
                    rootFound = True

        if not rootFound:
            raise Exception("Root directory of '" + name + "' container not specified in XML")

    def __init__(self, mainConfigPath):
        '''! Parses daemon's JSON configuration files.

         @param mainConfigPath    Path to the main config "daemon.conf"
        '''
        self.containers = {}
        print "Looking for container IDs..."

        # load main daemon JSON config file
        if not os.path.isfile(mainConfigPath):
            raise Exception(mainConfigPath + " not found. " +
                            "Please verify that security containers is properly installed.")
        with open(mainConfigPath) as daemonConfigStr:
            daemonConfigData = json.load(daemonConfigStr)
            daemonConfigDir = os.path.dirname(os.path.abspath(mainConfigPath))

            # get dictionary with containers
            containerConfigPaths = daemonConfigData["containerConfigs"]
            for configPath in containerConfigPaths:

                # open container config file
                containerConfigPath = os.path.join(daemonConfigDir, configPath)
                if not os.path.isfile(containerConfigPath):
                    raise Exception(containerConfigPath + " not found. " +
                                    "Please verify that security containers is properly installed.")
                with open(containerConfigPath) as containerConfigStr:
                    containerConfigData = json.load(containerConfigStr)

                    # extract XML config path for libvirt
                    libvirtConfigPath = os.path.join(daemonConfigDir, "containers",
                                                     containerConfigData["config"])

                    output, ret = sc_test_utils.launchProc("virt-xml-validate " + libvirtConfigPath)
                    if ret == 0:
                        self.__parseLibvirtXML(libvirtConfigPath)
                    else:
                        raise Exception(output)

'''! Module used to test zones' images completeness

@author: Michal Witanowski (m.witanowski@samsung.com)
'''
import unittest
from vsm_integration_tests.common import vsm_test_utils
from config_checker import *
import xml.etree.ElementTree as ET


# vasum daemon user name and user ID
VSM_USER_NAME = "security-containers"
VSM_UID = 377

DAEMON_DBUS_SOCKET_NAME = "org.tizen.vasum.zone"

# dbus config file path relative to zone's root
DBUS_CONFIG_PATH = "etc/dbus-1/system.d/" + DAEMON_DBUS_SOCKET_NAME + ".conf"

# main daemon config
DAEMON_CONFIG_PATH = "/etc/vasum/daemon.conf"


class ZoneImageTestCase(unittest.TestCase):
    '''! Test case class verifying zones' images
    '''

    @classmethod
    def setUpClass(self):
        '''! Sets up testing environment - collects zone names and paths.
        '''
        self.configChecker = ConfigChecker(DAEMON_CONFIG_PATH)

    def test01_vsmUserExistence(self):
        '''! Verifies if "vasum" user with an appropriate UID exists within the
             zones.
        '''
        for zoneName, zonePath in self.configChecker.zones.iteritems():
            # chroot into a zone and get UID of the user
            output, ret = vsm_test_utils.launchProc("chroot " + zonePath +
                                                   " /usr/bin/id -u " + VSM_USER_NAME)

            self.assertEqual(ret, 0, "User '" + VSM_USER_NAME + "' does not exist in '" +
                             zoneName + "' zone.")

            # cast to integer to remove white spaces, etc.
            uid = int(output)
            self.assertEqual(uid, VSM_UID, "Invalid UID of '" + VSM_USER_NAME + "' in '" +
                             zoneName + "' zone: got " + str(uid) +
                             ", should be " + str(VSM_UID))

    def test02_dbusConfig(self):
        '''! Verifies if dbus configuration file exists within zones.
        '''
        for zoneName, zonePath in self.configChecker.zones.iteritems():
            configPath = os.path.join(zonePath, DBUS_CONFIG_PATH)
            self.assertTrue(os.path.isfile(configPath), "Dbus configuration not found in '" +
                            zoneName + "' zone")

            tree = ET.parse(configPath)
            root = tree.getroot()
            self.assertEqual(root.tag, "busconfig", "Invalid root node name")

            # validate "vasum" access to the dbus
            ownCheck = False
            sendDestinationCheck = False
            receiveSenderCheck = False

            # extract directory mountings
            for elem in root.iterfind("policy"):
                if "user" not in elem.attrib:
                    continue

                self.assertEqual(elem.attrib["user"], VSM_USER_NAME, "dbus configuration allows '" +
                    elem.attrib["user"] + "' user to access the dbus socket.")

                for allowNode in elem.iterfind("allow"):
                    if "own" in allowNode.attrib:
                        ownCheck = (allowNode.attrib["own"] == DAEMON_DBUS_SOCKET_NAME)
                    if "send_destination" in allowNode.attrib:
                        sendDestinationCheck = (allowNode.attrib["send_destination"] ==
                            DAEMON_DBUS_SOCKET_NAME)
                    if "receive_sender" in allowNode.attrib:
                        receiveSenderCheck = (allowNode.attrib["receive_sender"] ==
                            DAEMON_DBUS_SOCKET_NAME)

            if not (ownCheck and sendDestinationCheck and sendDestinationCheck):
                raise Exception("Invalid dbus configuration in '" + zoneName + "' zone")

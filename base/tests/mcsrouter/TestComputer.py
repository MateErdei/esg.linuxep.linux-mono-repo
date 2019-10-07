#!/usr/bin/env python3


import unittest
import xml.dom.minidom
import mock

import logging
logger = logging.getLogger("TestComputer")

import PathManager

import mcsrouter.computer
import mcsrouter.adapters.agent_adapter

def getTargetSystem():
    import mcsrouter.targetsystem
    return mcsrouter.targetsystem.TargetSystem()

class TestcomputerCommonStatus(unittest.TestCase):
    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
    def testCommonStatusXml(self, *mockarg):
        adapter = mcsrouter.adapters.agent_adapter.AgentAdapter()
        status_xml = adapter.get_common_status_xml()

        ## Verify XML is valid
        doc = xml.dom.minidom.parseString(status_xml)
        doc.unlink()

class TestComputer(unittest.TestCase):
    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
    def testStatusXml(self, *mockarg):
        # ts = getTargetSystem()
        # if not ts.is_linux:
        #     return
        #
        c = mcsrouter.computer.Computer()
        #
        # computerStatus = c.getCommonStatusXml()

        adapter = mcsrouter.adapters.agent_adapter.AgentAdapter()
        status_xml = adapter.get_common_status_xml()

        logger.debug(status_xml)

        ## Verify it is XML
        doc = xml.dom.minidom.parseString(status_xml)
        doc.unlink()

        platformStatus = adapter.get_platform_status()
        logger.debug(platformStatus)
        doc = xml.dom.minidom.parseString(platformStatus)
        doc.unlink()

        ## Header isn't valid XML on its own
        logger.debug(adapter.get_status_header())


    def testPlatformStatusContainsOSVersionInformation(self):
        c = mcsrouter.computer.Computer()

        adapter = mcsrouter.adapters.agent_adapter.AgentAdapter()

        platformStatus = adapter.get_platform_status()
        logger.debug(platformStatus)
        doc = xml.dom.minidom.parseString(platformStatus)

        major_version_element = doc.getElementsByTagName("osMajorVersion")[0].firstChild.data
        minor_version_element = doc.getElementsByTagName("osMinorVersion")[0].firstChild.data

        #check the values being sent are valid numbers for a major and minor versions
        self.assertTrue(int(major_version_element) > 0)
        self.assertTrue(int(minor_version_element) > 0)
        doc.unlink()


    def testFormatIPv6(self):
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("00010000000000000000000000000000"),"1::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("11110000000000000000000000000000"),"1111::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("11112222000000000000000000000000"),"1111:2222::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("11110000222200000000000000000000"),"1111:0:2222::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("11110333222200000000000000000000"),"1111:333:2222::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("11110033222200000000000000000000"),"1111:33:2222::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("11110003222200000000000000000000"),"1111:3:2222::")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("fe80000000000000022186fffe29edfd"),"fe80::221:86ff:fe29:edfd")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("fe80000000000000025056fffec00008"),"fe80::250:56ff:fec0:8")

        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("20010db8000000000000000000000001"),"2001:db8::1")
        self.assertEqual(mcsrouter.adapters.agent_adapter.format_ipv6("20010db8000000010001000100010001"),"2001:db8:0:1:1:1:1:1")

    def testStatusDoesNotChange(self):
        c = mcsrouter.computer.Computer()
        self.assertFalse(c.has_status_changed())

if __name__ == '__main__':
    unittest.main()


#!/usr/bin/env python

from __future__ import absolute_import,print_function,division,unicode_literals

import unittest
import xml.dom.minidom

import logging
logger = logging.getLogger("TestComputer")

import PathManager

import mcsrouter.computer
import mcsrouter.adapters.agent_adapter

def getTargetSystem():
    import mcsrouter.targetsystem
    return mcsrouter.targetsystem.TargetSystem()

class TestcomputerCommonStatus(unittest.TestCase):
    # def testEqualsOperator(self):
    #     ts1 = getTargetSystem()
    #     ts2 = getTargetSystem()
    #     ccs1 = mcsrouter.computer.ComputerCommonStatus(ts1)
    #     ccs2 = mcsrouter.computer.ComputerCommonStatus(ts2)
    #     self.assertEqual(ccs1, ccs2)
    #
    # ## the above and below tests don't make sense. They test the inbuilt unit test methods.
    #
    # def testNotEqualsOperator(self):
    #     ## old
    #     ts1 = getTargetSystem()
    #     ts2 = getTargetSystem()
    #     ccs1 = mcsrouter.computer.ComputerCommonStatus(ts1)
    #     ccs2 = mcsrouter.computer.ComputerCommonStatus(ts2)
    #     ccs2.computerName = "Something different"
    #     self.assertNotEqual(ccs1, ccs2)
    #
    #     ## in progress new
    #     # adapter1 = mcsrouter.adapters.agent_adapter.AgentAdapter()
    #     # status_xml1 = adapter1.get_common_status_xml()
    #     # adapter2 = mcsrouter.adapters.agent_adapter.AgentAdapter()
    #     # status_xml2 = adapter2.get_common_status_xml()
    #     # self.assertNotEqual(status_xml1, status_xml2)




    def testCommonStatusXml(self):
        adapter = mcsrouter.adapters.agent_adapter.AgentAdapter()
        status_xml = adapter.get_common_status_xml()

        ## Verify XML is valid
        doc = xml.dom.minidom.parseString(status_xml)
        doc.unlink()

class TestComputer(unittest.TestCase):
    def testStatusXml(self):
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


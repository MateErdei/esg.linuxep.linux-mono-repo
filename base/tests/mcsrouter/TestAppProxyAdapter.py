#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import unittest
import sys
import os

import PathManager

import xml.dom.minidom

import mcsrouter.adapters.app_proxy_adapter
import mcsrouter.mcsclient.mcs_commands as MCSCommands

builddir="."

BUILD_DIR=os.environ.get("ABS_BUILDDIR",".")
BUILD_DIR=os.path.abspath(BUILD_DIR)
TEST_DIR=os.path.join(BUILD_DIR,"test")

POLICY_ASSIGNMENTS="""<?xml version="1.0" encoding="utf-8"?>
<ns:policyAssignments xmlns:ns="http://www.sophos.com/xml/mcs/policyAssignments">
    <meta protocolVersion="1.0"/>
    <policyAssignment>
        <appId>SAV</appId>
        <policyId>9db6354d-c5e4-4af4-b512-a1f5860f419a</policyId>
    </policyAssignment>
</ns:policyAssignments>"""

POLICY_ASSIGNMENTS_ADDITIONAL_TAGS="""<?xml version="1.0" encoding="utf-8"?>
<ns:policyAssignments xmlns:ns="http://www.sophos.com/xml/mcs/policyAssignments">
    <meta protocolVersion="1.0"/>
    <bogus>
        <policyAssignment>
            <appId/>
            <policyId/>
        </policyAssignment>
    </bogus>
    <policyAssignment>
        <bogus>
            <appId>HBT</appId>
        </bogus>
        <appId>SAV</appId>
        <policyId>9db6354d-c5e4-4af4-b512-a1f5860f419a</policyId>
    </policyAssignment>
</ns:policyAssignments>"""

FRAGMENTS="""<?xml version="1.0"?>
<ns:policyFragments xmlns:ns="http://www.sophos.com/xml/mcs/policyFragments">
    <meta protocolVersion="1.0"/>
    <fragments>
        <appId>HBT</appId>
        <fragment seq="0" id="248a51ecac446bdfa3ff56d8f2bb8ad64d29ffa142786816dcade032782483a4"/>
        <fragment seq="1" id="e6da4cf3ebbc0797010174b613b10f59c00d9aa49822a45dc70929c9bc5db003"/>
        <fragment seq="2" id="77e46fdd793bb1a4e442ee0149170619b0f275759e342f7c63138a442d2d3891"/>
    </fragments>
</ns:policyFragments>"""

FRAGMENTS_ADDITIONAL_TAGS="""<?xml version="1.0"?>
<ns:policyFragments xmlns:ns="http://www.sophos.com/xml/mcs/policyFragments">
    <meta protocolVersion="1.0"/>
    <bogus>
        <fragments>
            <appId/>
            <fragment/>
        </fragments>
    </bogus>
    <fragments>
        <appId>HBT</appId>
        <fragment seq="0" id="248a51ecac446bdfa3ff56d8f2bb8ad64d29ffa142786816dcade032782483a4"/>
        <fragment seq="1" id="e6da4cf3ebbc0797010174b613b10f59c00d9aa49822a45dc70929c9bc5db003"/>
        <bogus>
            <fragment id="blah"/>
        </bogus>
        <fragment seq="2" id="77e46fdd793bb1a4e442ee0149170619b0f275759e342f7c63138a442d2d3891"/>
    </fragments>
</ns:policyFragments>"""

INVALID_XML="""<?xml version="1.0"?>
<ns:policyFragments xmlns:ns="http://www.sophos.com/xml/mcs/policyFragments">
    <meta protocolVersion="1.0"/>
    <fragments>
        <appId>HBT</appId>
        <unclosed tag>
        <fragment seq="0" id="248a51ecac446bdfa3ff56d8f2bb8ad64d29ffa142786816dcade032782483a4"/>
        <fragment seq="1" id="e6da4cf3ebbc0797010174b613b10f59c00d9aa49822a45dc70929c9bc5db003"/>
        <fragment seq="2" id="77e46fdd793bb1a4e442ee0149170619b0f275759e342f7c63138a442d2d3891"/>
    </fragments>
</ns:policyFragments>"""

class Connection(object):
    def __init__(self):
        self.__m_appId = ""
        self.__m_policyId = ""

    def getPolicy(self, appId, policyId):
        self.__m_appId = appId
        self.__m_policyId = policyId

    def getAppId(self):
        return self.__m_appId

    def getPolicyId(self):
        return self.__m_policyId

    def getPolicyFragment(self, appId, fragmentId):
        return "{}|{}|".format(appId, fragmentId)


class FakeCommand(object):
    def __init__(self,xml):
        self._m__dict = {"body":xml}
        self._m__complete = False
        self._m__connection = Connection()

    def getConnection(self):
        return self._m__connection

    def get(self, s):
        return self._m__dict.get(s, "")

    def getXmlText(self):
        return ""

    def complete(self):
        self._m__complete = True

    def completed(self):
        return self.completed


class TestAppProxyAdapter(unittest.TestCase):
    def setUp(self):
        os.chdir(TEST_DIR)
        PathManager.safeMkdir(os.path.join(BUILD_DIR,"install","var", "cache", "mcs_fragmented_policies"))

    def tearDown(self):
        os.chdir(BUILD_DIR)
        PathManager.safeDelete(os.path.join(BUILD_DIR,"install","var", "cache", "mcs_fragmented_policies", "*"))

    def testPolicyAssignmentsParsedCorrectly(self):
        command = FakeCommand(POLICY_ASSIGNMENTS)
        policyCommand = mcsrouter.adapters.app_proxy_adapter.AppProxyAdapter().processCommand(command)
        self.assertEqual(len(policyCommand), 1)
        self.assertEqual(policyCommand[0].getAppId(), "SAV")
        policyCommand[0].getPolicy()
        self.assertEqual(command.getConnection().getPolicyId(), "9db6354d-c5e4-4af4-b512-a1f5860f419a")
        self.assertTrue(command.completed)

    def testPolicyFragmentsParsedCorrectly(self):
        command = FakeCommand(FRAGMENTS)
        policyCommand = mcsrouter.adapters.app_proxy_adapter.AppProxyAdapter().processCommand(command)
        self.assertEqual(len(policyCommand), 1)
        self.assertEqual(policyCommand[0].getAppId(), "HBT")
        # getPolicy output formatted by Connection.getPolicyFragment above
        expectedFragmentsString = "HBT|248a51ecac446bdfa3ff56d8f2bb8ad64d29ffa142786816dcade032782483a4|"
        expectedFragmentsString += "HBT|e6da4cf3ebbc0797010174b613b10f59c00d9aa49822a45dc70929c9bc5db003|"
        expectedFragmentsString += "HBT|77e46fdd793bb1a4e442ee0149170619b0f275759e342f7c63138a442d2d3891|"
        self.assertEqual(policyCommand[0].getPolicy(), expectedFragmentsString)
        self.assertTrue(command.completed)

    def testPolicyAssignmentsIgnoreAdditionalTags(self):
        command = FakeCommand(POLICY_ASSIGNMENTS_ADDITIONAL_TAGS)
        policyCommand = mcsrouter.adapters.app_proxy_adapter.AppProxyAdapter().processCommand(command)
        self.assertEqual(len(policyCommand), 1)
        self.assertEqual(policyCommand[0].getAppId(), "SAV")
        policyCommand[0].getPolicy()
        self.assertEqual(command.getConnection().getPolicyId(), "9db6354d-c5e4-4af4-b512-a1f5860f419a")
        self.assertTrue(command.completed)

    def testPolicyFragmentsIgnoreAdditionalTags(self):
        command = FakeCommand(FRAGMENTS_ADDITIONAL_TAGS)
        policyCommand = mcsrouter.adapters.app_proxy_adapter.AppProxyAdapter().processCommand(command)
        self.assertEqual(len(policyCommand), 1)
        self.assertEqual(policyCommand[0].getAppId(), "HBT")
        # getPolicy output formatted by Connection.getPolicyFragment above
        expectedFragmentsString = "HBT|248a51ecac446bdfa3ff56d8f2bb8ad64d29ffa142786816dcade032782483a4|"
        expectedFragmentsString += "HBT|e6da4cf3ebbc0797010174b613b10f59c00d9aa49822a45dc70929c9bc5db003|"
        expectedFragmentsString += "HBT|77e46fdd793bb1a4e442ee0149170619b0f275759e342f7c63138a442d2d3891|"
        self.assertEqual(policyCommand[0].getPolicy(), expectedFragmentsString)
        self.assertTrue(command.completed)

    def testInvalidXml(self):
        command = FakeCommand(INVALID_XML)
        policyCommand = mcsrouter.adapters.app_proxy_adapter.AppProxyAdapter().processCommand(command)
        self.assertEqual(policyCommand, [])
        self.assertTrue(command.completed)


if __name__ == '__main__':
    unittest.main()

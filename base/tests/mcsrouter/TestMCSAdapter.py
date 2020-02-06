#!/usr/bin/env python

import unittest
import sys
import os
import xml.dom.minidom
import PathManager

import logging
logger = logging.getLogger(__name__)

import mcsrouter.adapters.mcs_adapter
import mcsrouter.utils.config

import builtins

builtins.__dict__['REGISTER_MCS'] = False

FakePolicyCommand = PathManager.FakePolicyCommand
FakeConfigManager = PathManager.FakeConfigManager
setFakeConfigManager = PathManager.setFakeConfigManager

def createAdapter(policy_config=None,applied_config=None):
    if policy_config is None:
        policy_config = mcsrouter.utils.config.Config()
    if applied_config is None:
        applied_config = policy_config

    return mcsrouter.adapters.mcs_adapter.MCSAdapter("install",
                                                      policy_config,
                                                      applied_config)

SIMPLE_MCS_POLICY="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="6f9cb63e8cb1eaa98df9efbf5d218056668f5f34392ba53cf206af03d7eb6614" policyType="25"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>21e72d59248ce0dc5f957b659d72a4261b663041bbc96c54a81c8ea578186648</registrationToken>
    <servers>
      <server>https://mcs.sandbox.sophos/sophos/management/ep</server>
    </servers>
    <useSystemProxy>true</useSystemProxy>
    <useAutomaticProxy>true</useAutomaticProxy>
    <useDirect>true</useDirect>
    <randomSkewFactor>1</randomSkewFactor>
    <commandPollingDelay default="20"/>
    <policyChangeServers/>
  </configuration>
</policy>"""

MCS_POLICY_WITH_PROXIES="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="6f9cb63e8cb1eaa98df9efbf5d218056668f5f34392ba53cf206af03d7eb6614" policyType="25"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>21e72d59248ce0dc5f957b659d72a4261b663041bbc96c54a81c8ea578186648</registrationToken>
    <servers>
      <server>https://mcs.sandbox.sophos/sophos/management/ep</server>
    </servers>
     <proxies>
      <proxy>http://some.proxy.sophos:8080</proxy>
    </proxies>
    <proxyCredentials>
      <credentials>passwordAndUsername</credentials>
    </proxyCredentials>
    <useSystemProxy>true</useSystemProxy>
    <useAutomaticProxy>true</useAutomaticProxy>
    <useDirect>true</useDirect>
    <randomSkewFactor>1</randomSkewFactor>
    <commandPollingDelay default="20"/>
    <policyChangeServers/>
  </configuration>
</policy>"""

EXAMPLE_MCS_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="{{revId}}" policyType="25"/>
  <configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <registrationToken>{{registrationToken}}</registrationToken>
    <servers>
      <server>{{server}}</server>
    </servers>
    <messageRelays>
      <messageRelay priority="{{priority}}" address="{{address}}" port="{{port}}" id="{{endpointId}}"/>
    </messageRelays>
    <proxies>
      <proxy>{{proxy}}</proxy>
    </proxies>
    <proxyCredentials>
      <credentials>{{SECObfuscatedCredentials}}</credentials>
    </proxyCredentials>
    <useDirect>true</useDirect>
    <useSystemProxy>true</useSystemProxy>
    <useAutomaticProxy>true</useAutomaticProxy>
    <commandPollingDelay default="17"/>
    <diagnosticTrailEnabled>true</diagnosticTrailEnabled>
  </configuration>
</policy>"""

def extractCompliance(xmltext):
    dom = xml.dom.minidom.parseString(xmltext)
    try:
        compNodes = dom.getElementsByTagName("csc:CompRes")
        assert len(compNodes) == 1
        compNode = compNodes[0]
        return (compNode.getAttribute("Res"),compNode.getAttribute("RevID"),compNode.getAttribute("policyType"))
    finally:
        dom.unlink()

class TestMCSAdapter(unittest.TestCase):
    def setup(self):
        PathManager.safeMkdir("install/rms")
        PathManager.safeMkdir("install/tmp")

    def tearDown(self):
        #PathManager.resetConfigManager()
        PathManager.safeDelete("install/rms/mcsPolicy.xml")
        PathManager.safeDelete("install/tmp/mcsPolicy.xml")
        PathManager.safeDelete("base/etc/sophosspl/mcs_policy.config")

    def testCreation(self):
        adapter = createAdapter()

    # def testApplyPolicy(self):
    #     policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
    #     configManager = setFakeConfigManager()
    #     adapter = createAdapter(policy_config,policy_config)
    #     command = FakePolicyCommand(SIMPLE_MCS_POLICY)
    #     adapter.process_command(command)
    #     self.assertTrue(command.m_complete)
    #     self.assertTrue(os.path.isfile("install/tmp/mcsPolicy.xml"))
    #     self.assertTrue(os.path.isfile("install/rms/mcsPolicy.xml"))
    #     self.assertTrue(policy_config.getBool("useSystemProxy",False))
    #     self.assertTrue(policy_config.getBool("useAutomaticProxy",False))
    #     self.assertTrue(policy_config.getBool("useDirect",False))
    #     self.assertTrue(policy_config.getDefault("MCSToken",""),"21e72d59248ce0dc5f957b659d72a4261b663041bbc96c54a81c8ea578186648")
    #     self.assertEqual(policy_config.get_int("COMMAND_CHECK_INTERVAL_MINIMUM",0),20)
    #     self.assertEqual(policy_config.get_int("COMMAND_CHECK_INTERVAL_MAXIMUM",0),20)
    #     self.assertEqual(policy_config.getDefault("mcs_policy_url1",""),"https://mcs.sandbox.sophos/sophos/management/ep")
    #     self.assertTrue(os.path.isfile("base/etc/sophosspl/mcs_policy.config"))
    #     (res,revID,policyType) = extractCompliance(adapter._getStatusXml())
    #     self.assertEqual(policyType,"25")
    #     self.assertEqual(res,"Same")
    #     self.assertEqual(revID,"6f9cb63e8cb1eaa98df9efbf5d218056668f5f34392ba53cf206af03d7eb6614")

#     def testEmptyPolicy(self):
#         TEST_CONFIG="""<?xml version="1.0"?>
# <policy xmlns:csc="com.sophos\msys\csc" type="mcs">
# </policy>
# """
#         policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
#         configManager = setFakeConfigManager()
#         adapter = createAdapter(policy_config)
#         command = FakePolicyCommand(TEST_CONFIG)
#         adapter.process_command(command)
#         self.assertTrue(command.m_complete)
#         self.assertTrue(os.path.isfile("install/tmp/mcsPolicy.xml"))
#         self.assertTrue(os.path.isfile("install/rms/mcsPolicy.xml"))
#         self.assertEqual(policy_config.getBool("useSystemProxy",None),None)
#         self.assertTrue(policy_config.getBool("useAutomaticProxy",None) is None)
#         self.assertTrue(policy_config.getBool("useDirect",None) is None)
#         self.assertEqual(policy_config.get_int("COMMAND_CHECK_INTERVAL_MINIMUM",1337),1337)
#         self.assertEqual(policy_config.get_int("COMMAND_CHECK_INTERVAL_MAXIMUM",1337),1337)

#     def testTmpFileContainsPolicy(self):
#         TEST_CONFIG="""<?xml version="1.0"?>
# <policy xmlns:csc="com.sophos\msys\csc" type="mcs">
# </polidasdasdasdas>
# """
#         configManager = setFakeConfigManager()
#         adapter = createAdapter()
#         command = FakePolicyCommand(TEST_CONFIG)
#         adapter.process_command(command)
#         self.assertTrue(command.m_complete)
#         self.assertTrue(os.path.isfile("install/tmp/mcsPolicy.xml"))
#         self.assertFalse(os.path.isfile("install/rms/mcsPolicy.xml"))
#         actual = open("install/tmp/mcsPolicy.xml").read()
#         self.assertEqual(actual,TEST_CONFIG)

    # def testPolicyLoadedAtCreation(self):
    #     policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
    #     PathManager.writeFile("install/rms/mcsPolicy.xml",SIMPLE_MCS_POLICY)
    #     adapter = createAdapter(policy_config)
    #     self.assertTrue(policy_config.getBool("useSystemProxy",False))
    #     self.assertTrue(policy_config.getBool("useAutomaticProxy",False))
    #     self.assertTrue(policy_config.getBool("useDirect",False))
    #     self.assertEqual(policy_config.get_int("COMMAND_CHECK_INTERVAL_MINIMUM",0),20)
    #     self.assertEqual(policy_config.get_int("COMMAND_CHECK_INTERVAL_MAXIMUM",0),20)

    # def testParsingExamplePolicy(self):
    #     policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
    #     configManager = setFakeConfigManager()
    #     adapter = createAdapter(policy_config)
    #     command = FakePolicyCommand(EXAMPLE_MCS_POLICY)
    #     adapter.process_command(command)
    #     self.assertTrue(command.m_complete)
    #     self.assertTrue(os.path.isfile("install/tmp/mcsPolicy.xml"))
    #     self.assertTrue(os.path.isfile("install/rms/mcsPolicy.xml"))
    #     self.assertEqual(policy_config.getDefault("MCSToken",""),"{{registrationToken}}")

    # def testParsingPolicyWithProxies(self):
    #     policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
    #     configManager = setFakeConfigManager()
    #     adapter = createAdapter(policy_config)
    #     command = FakePolicyCommand(MCS_POLICY_WITH_PROXIES)
    #     adapter.process_command(command)
    #     self.assertTrue(command.m_complete)
    #     self.assertTrue(os.path.isfile("install/tmp/mcsPolicy.xml"))
    #     self.assertTrue(os.path.isfile("install/rms/mcsPolicy.xml"))
    #     self.assertEqual(policy_config.getDefault("mcs_policy_proxy",""),"http://some.proxy.sophos:8080")
    #     self.assertEqual(policy_config.getDefault("mcs_policy_proxy_credentials",""),"passwordAndUsername")

    # def testParsingPolicyWithUnsetProxies(self):
    #     # Set proxies in the policy
    #     policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
    #     configManager = setFakeConfigManager()
    #     adapter = createAdapter(policy_config)
    #     command = FakePolicyCommand(MCS_POLICY_WITH_PROXIES)
    #     adapter.process_command(command)
    #     self.assertTrue(command.m_complete)
    #     self.assertTrue(os.path.isfile("install/tmp/mcsPolicy.xml"))
    #     self.assertTrue(os.path.isfile("install/rms/mcsPolicy.xml"))
    #     self.assertEqual(policy_config.getDefault("mcs_policy_proxy",""),"http://some.proxy.sophos:8080")
    #     self.assertEqual(policy_config.getDefault("mcs_policy_proxy_credentials",""),"passwordAndUsername")
    #
    #     #Remove proxies from the policy, check that they're removed from config
    #     command = FakePolicyCommand(SIMPLE_MCS_POLICY)
    #     adapter.process_command(command)
    #     self.assertTrue(command.m_complete)
    #     self.assertTrue(os.path.isfile("install/tmp/mcsPolicy.xml"))
    #     self.assertTrue(os.path.isfile("install/rms/mcsPolicy.xml"))
    #     self.assertEqual(policy_config.getDefault("mcs_policy_proxy",""),"")
    #     self.assertEqual(policy_config.getDefault("mcs_policy_proxy_credentials",""),"")

    # def testNonCompliantURL(self):
    #     policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
    #     applied_config = mcsrouter.utils.config.Config("install/etc/sophosav/mcs.config",policy_config)
    #     configManager = setFakeConfigManager()
    #     adapter = createAdapter(policy_config,applied_config)
    #     command = FakePolicyCommand(SIMPLE_MCS_POLICY)
    #     adapter.process_command(command)
    #     self.assertEqual(policy_config.getDefault("mcs_policy_url1",""),"https://mcs.sandbox.sophos/sophos/management/ep")
    #
    #     applied_config.set("mcs_policy_url1","https://notmcs")
    #
    #     (res,revID,policyType) = extractCompliance(adapter._getStatusXml())
    #     self.assertEqual(policyType,"25")
    #     self.assertEqual(res,"Diff")
    #     self.assertEqual(revID,"6f9cb63e8cb1eaa98df9efbf5d218056668f5f34392ba53cf206af03d7eb6614")

    # def testNonCompliantPollingInterval(self):
    #     policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
    #     applied_config = mcsrouter.utils.config.Config("install/etc/sophosav/mcs.config",policy_config)
    #     configManager = setFakeConfigManager()
    #     adapter = createAdapter(policy_config,applied_config)
    #     command = FakePolicyCommand(SIMPLE_MCS_POLICY)
    #     adapter.process_command(command)
    #     self.assertEqual(policy_config.get_int("COMMAND_CHECK_INTERVAL_MINIMUM",0),20)
    #     self.assertEqual(policy_config.get_int("COMMAND_CHECK_INTERVAL_MAXIMUM",0),20)
    #
    #     applied_config.set("COMMAND_CHECK_INTERVAL_MINIMUM","5")
    #     applied_config.set("COMMAND_CHECK_INTERVAL_MAXIMUM","5")
    #
    #     (res,revID,policyType) = extractCompliance(adapter._getStatusXml())
    #     self.assertEqual(policyType,"25")
    #     self.assertEqual(res,"Diff")
    #     self.assertEqual(revID,"6f9cb63e8cb1eaa98df9efbf5d218056668f5f34392ba53cf206af03d7eb6614")

    def testExtraCommandPollingDelayTagIgnored(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <csc:Comp RevID="5" policyType="25"/>
    <commandPollingDelay default="123"/>
    <bogus>
    <commandPollingDelay default="321"/>
    </bogus>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        policy_config.set("COMMAND_CHECK_INTERVAL_MINIMUM","15")
        policy_config.set("COMMAND_CHECK_INTERVAL_MAXIMUM","15")

        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(123, policy_config.get_int("COMMAND_CHECK_INTERVAL_MINIMUM", 0))
        self.assertEqual(14400, policy_config.get_int("COMMAND_CHECK_INTERVAL_MAXIMUM", 0))

    def testNoAttributeInPollingElement(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <commandPollingDelay/>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        policy_config.set("COMMAND_CHECK_INTERVAL_MINIMUM","15")
        policy_config.set("COMMAND_CHECK_INTERVAL_MAXIMUM","15")

        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_int("COMMAND_CHECK_INTERVAL_MINIMUM",0),15)
        self.assertEqual(policy_config.get_int("COMMAND_CHECK_INTERVAL_MAXIMUM",0),15)

    def testNonIntAttributeInPollingElement(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <commandPollingDelay default="FooBar"/>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        policy_config.set("COMMAND_CHECK_INTERVAL_MINIMUM","15")
        policy_config.set("COMMAND_CHECK_INTERVAL_MAXIMUM","15")

        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_int("COMMAND_CHECK_INTERVAL_MINIMUM",0),15)
        self.assertEqual(policy_config.get_int("COMMAND_CHECK_INTERVAL_MAXIMUM",0),15)

    def testNoServerInServersElement(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <servers>
    </servers>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        policy_config.set("MCSURL","FOOBAR")

        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("MCSURL",None),"FOOBAR")

    def testMultipleServerInServersElement(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <csc:Comp RevID="5" policyType="25"/>
    <servers>
      <server>https://mcs1.sandbox.sophos/sophos/management/ep</server>
      <server>https://mcs2.sandbox.sophos/sophos/management/ep</server>
    </servers>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        policy_config.set("MCSURL","FOOBAR")

        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("mcs_policy_url1",None),"https://mcs1.sandbox.sophos/sophos/management/ep")
        self.assertEqual(policy_config.get_default("mcs_policy_url2",None),"https://mcs2.sandbox.sophos/sophos/management/ep")
        self.assertEqual(policy_config.get_default("MCSURL",None),"FOOBAR")

    def testMCSPushServerElements(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <csc:Comp RevID="5" policyType="25"/>
        <pushServers>
          <pushServer>https://push-loadbalancer-prod-eu-west1.sophos.com</pushServer>
        </pushServers>
        <pushPingTimeout>90</pushPingTimeout>
        <pushFallbackPollInterval>120</pushFallbackPollInterval>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        policy_config.set("MCSURL","FOOBAR")

        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("pushServer1", None), "https://push-loadbalancer-prod-eu-west1.sophos.com")
        self.assertEqual(policy_config.get_int("pushPingTimeout", 0), 90)
        self.assertEqual(policy_config.get_int("pushFallbackPollInterval", 0), 120)
        self.assertEqual(policy_config.get_default("MCSURL", None), "FOOBAR")

    def testMultiplePushServersInPushServersElement(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <csc:Comp RevID="5" policyType="25"/>
        <pushServers>
          <pushServer>https://push-loadbalancer-prod-eu-west1.sophos.com</pushServer>
          <pushServer>https://push-loadbalancer-prod-eu-west2.sophos.com</pushServer>
        </pushServers>

</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        policy_config.set("MCSURL","FOOBAR")

        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("pushServer1", None), "https://push-loadbalancer-prod-eu-west1.sophos.com")
        self.assertEqual(policy_config.get_default("pushServer2", None), "https://push-loadbalancer-prod-eu-west2.sophos.com")
        self.assertEqual(policy_config.get_int("pushPingTimeout", 0), 0)
        self.assertEqual(policy_config.get_int("pushFallbackPollInterval", 0), 0)

    def testNoPushServersInPushServersElement(self):
        TEST_POLICY="""<?xml version="1.0"?>
    <policy xmlns:csc="com.sophos\msys\csc" type="mcs">
    <configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <csc:Comp RevID="5" policyType="25"/>
        <pushServers>
        </pushServers>
    
    </configuration>
    </policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        policy_config.set("MCSURL","FOOBAR")

        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("pushServer1", ""), "")

    def testNoProxyInProxiesElement(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <csc:Comp RevID="5" policyType="25"/>
     <proxies>
    </proxies>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        policy_config.set("mcs_policy_proxy","FOOBAR")

        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("mcs_policy_proxy",None),None)

    def testMultipleProxyInProxiesElement(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
     <csc:Comp RevID="5" policyType="25"/>
     <proxies>
      <proxy>http://1.proxy.sophos:8080</proxy>
      <proxy>http://2.proxy.sophos:8080</proxy>
    </proxies>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        policy_config.set("mcs_policy_proxy","FOOBAR")

        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("mcs_policy_proxy",None),"http://1.proxy.sophos:8080")

    def testNoCredentialsInProxyCredentialsElement(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <csc:Comp RevID="5" policyType="25"/>
     <proxies>
      <proxy>http://1.proxy.sophos:8080</proxy>
    </proxies>
    <proxyCredentials>
    </proxyCredentials>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        policy_config.set("mcs_policy_proxy","FOOBAR")
        policy_config.set("mcs_policy_proxy_credentials","FOOBAR")

        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("mcs_policy_proxy",None),"http://1.proxy.sophos:8080")
        self.assertEqual(policy_config.get_default("mcs_policy_proxy_credentials",None),"FOOBAR")

    def testMultipleCredentialsInProxyCredentialsElement(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <csc:Comp RevID="5" policyType="25"/>
    <proxies>
      <proxy>http://1.proxy.sophos:8080</proxy>
    </proxies>
    <proxyCredentials>
      <credentials>passwordAndUsername1</credentials>
      <credentials>passwordAndUsername2</credentials>
    </proxyCredentials>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        policy_config.set("mcs_policy_proxy","FOOBAR")
        policy_config.set("mcs_policy_proxy_credentials","FOOBAR")

        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("mcs_policy_proxy",None),"http://1.proxy.sophos:8080")
        self.assertEqual(policy_config.get_default("mcs_policy_proxy_credentials",None),"passwordAndUsername1")

    def testNonPolicyRootNode(self):
        TEST_POLICY="""<?xml version="1.0"?>
<foobar xmlns:csc="com.sophos\msys\csc" type="mcs">
</foobar>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

    def testReducingNumberOfUrls(self):
        TEST_POLICY_1="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <csc:Comp RevID="5" policyType="25"/>
    <servers>
      <server>https://mcs1.sandbox.sophos/sophos/management/ep</server>
      <server>https://mcs2.sandbox.sophos/sophos/management/ep</server>
    </servers>
</configuration>
</policy>"""
        TEST_POLICY_2="""<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
    <csc:Comp RevID="5" policyType="25"/>
    <servers>
      <server>https://mcs2.sandbox.sophos/sophos/management/ep</server>
    </servers>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY_1)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("mcs_policy_url1",None),"https://mcs1.sandbox.sophos/sophos/management/ep")
        self.assertEqual(policy_config.get_default("mcs_policy_url2",None),"https://mcs2.sandbox.sophos/sophos/management/ep")


        command = FakePolicyCommand(TEST_POLICY_2)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)
        self.assertEqual(policy_config.get_default("mcs_policy_url1",None),"https://mcs2.sandbox.sophos/sophos/management/ep")
        self.assertEqual(policy_config.get_default("mcs_policy_url2",None),None)

    def testMessageRelayDetails(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay priority="1" port="8190" address="WIN-QS4JOM4FNKK" id="e5fe429b-c478-42f1-aed4-6b891d21b5b8"/>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),"1")
        self.assertEqual(policy_config.get_default("message_relay_port1",None),"8190")
        self.assertEqual(policy_config.get_default("message_relay_address1",None),"WIN-QS4JOM4FNKK")
        self.assertEqual(policy_config.get_default("message_relay_id1",None),"e5fe429b-c478-42f1-aed4-6b891d21b5b8")

    def testMultipleMessageRelays(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay priority="1" port="8190" address="WIN-QS4JOM4FNKK" id="e5fe429b-c478-42f1-aed4-6b891d21b5b8"/>
      <messageRelay priority="0" port="8190" address="LOSE-QS4JOM4FNKK" id="e5fethan-c478-42f1-alex-6b891d21b5b8"/>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),"1")
        self.assertEqual(policy_config.get_default("message_relay_port1",None),"8190")
        self.assertEqual(policy_config.get_default("message_relay_address1",None),"WIN-QS4JOM4FNKK")
        self.assertEqual(policy_config.get_default("message_relay_id1",None),"e5fe429b-c478-42f1-aed4-6b891d21b5b8")

        self.assertEqual(policy_config.get_default("message_relay_priority2",None),"0")
        self.assertEqual(policy_config.get_default("message_relay_port2",None),"8190")
        self.assertEqual(policy_config.get_default("message_relay_address2",None),"LOSE-QS4JOM4FNKK")
        self.assertEqual(policy_config.get_default("message_relay_id2",None),"e5fethan-c478-42f1-alex-6b891d21b5b8")

    def testMessageRelayNoMessageRelay(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_port1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_address1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_id1",None),None)

    def testMessageRelayEmptyPriority(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay priority="" port="8190" address="WIN-QS4JOM4FNKK" id="e5fe429b-c478-42f1-aed4-6b891d21b5b8"/>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_port1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_address1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_id1",None),None)

    def testMessageRelayMissingPriority(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay port="8190" address="WIN-QS4JOM4FNKK" id="e5fe429b-c478-42f1-aed4-6b891d21b5b8"/>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_port1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_address1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_id1",None),None)

    def testMessageRelayEmptyPort(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay priority="1" port="" address="WIN-QS4JOM4FNKK" id="e5fe429b-c478-42f1-aed4-6b891d21b5b8"/>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_port1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_address1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_id1",None),None)

    def testMessageRelayMissingPort(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay priority="1" address="WIN-QS4JOM4FNKK" id="e5fe429b-c478-42f1-aed4-6b891d21b5b8"/>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_port1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_address1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_id1",None),None)

    def testMessageRelayEmptyAddress(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay priority="1" port="8190" address="" id="e5fe429b-c478-42f1-aed4-6b891d21b5b8"/>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_port1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_address1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_id1",None),None)

    def testMessageRelayMissingAddress(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay priority="1" port="8190" id="e5fe429b-c478-42f1-aed4-6b891d21b5b8"/>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_port1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_address1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_id1",None),None)

    def testMessageRelayEmptyId(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay priority="1" port="8190" address="WIN-QS4JOM4FNKK" id=""/>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_port1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_address1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_id1",None),None)

    def testMessageRelayMissingId(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay priority="1" port="8190" address="WIN-QS4JOM4FNKK"/>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_port1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_address1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_id1",None),None)

    def testBrokenAndFunctionalMessageRelay(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay priority="1" address="WIN-QS4JOM4FNKK" id="e5fe429b-c478-42f1-aed4-6b891d21b5b8"/>
      <messageRelay priority="0" port="8190" address="LOSE-QS4JOM4FNKK" id="e5fethan-c478-42f1-alex-6b891d21b5b8"/>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),"0")
        self.assertEqual(policy_config.get_default("message_relay_port1",None),"8190")
        self.assertEqual(policy_config.get_default("message_relay_address1",None),"LOSE-QS4JOM4FNKK")
        self.assertEqual(policy_config.get_default("message_relay_id1",None),"e5fethan-c478-42f1-alex-6b891d21b5b8")

    def testReducingNumberOfMessageRelays(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay priority="1" port="8190" address="WIN-QS4JOM4FNKK" id="e5fe429b-c478-42f1-aed4-6b891d21b5b8"/>
      <messageRelay priority="0" port="8190" address="LOSE-QS4JOM4FNKK" id="e5fethan-c478-42f1-alex-6b891d21b5b8"/>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),"1")
        self.assertEqual(policy_config.get_default("message_relay_port1",None),"8190")
        self.assertEqual(policy_config.get_default("message_relay_address1",None),"WIN-QS4JOM4FNKK")
        self.assertEqual(policy_config.get_default("message_relay_id1",None),"e5fe429b-c478-42f1-aed4-6b891d21b5b8")

        self.assertEqual(policy_config.get_default("message_relay_priority2",None),"0")
        self.assertEqual(policy_config.get_default("message_relay_port2",None),"8190")
        self.assertEqual(policy_config.get_default("message_relay_address2",None),"LOSE-QS4JOM4FNKK")
        self.assertEqual(policy_config.get_default("message_relay_id2",None),"e5fethan-c478-42f1-alex-6b891d21b5b8")

        TEST_POLICY2="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay priority="0" port="1066" address="WIN-QS4JOM4FNKK" id="e5fe429b-c478-42f1-aed4-6b891d21b5b8"/>
    </messageRelays>
</configuration>
</policy>"""
        command = FakePolicyCommand(TEST_POLICY2)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),"0")
        self.assertEqual(policy_config.get_default("message_relay_port1",None),"1066")
        self.assertEqual(policy_config.get_default("message_relay_address1",None),"WIN-QS4JOM4FNKK")
        self.assertEqual(policy_config.get_default("message_relay_id1",None),"e5fe429b-c478-42f1-aed4-6b891d21b5b8")

        self.assertEqual(policy_config.get_default("message_relay_priority2",None),None)
        self.assertEqual(policy_config.get_default("message_relay_port2",None),None)
        self.assertEqual(policy_config.get_default("message_relay_address2",None),None)
        self.assertEqual(policy_config.get_default("message_relay_id2",None),None)

    def testReducingNumberOfMessageRelaystoZero(self):
        TEST_POLICY="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
      <messageRelay priority="1" port="8190" address="WIN-QS4JOM4FNKK" id="e5fe429b-c478-42f1-aed4-6b891d21b5b8"/>
      <messageRelay priority="0" port="8190" address="LOSE-QS4JOM4FNKK" id="e5fethan-c478-42f1-alex-6b891d21b5b8"/>
    </messageRelays>
</configuration>
</policy>"""
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")
        adapter = createAdapter(policy_config)

        command = FakePolicyCommand(TEST_POLICY)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),"1")
        self.assertEqual(policy_config.get_default("message_relay_port1",None),"8190")
        self.assertEqual(policy_config.get_default("message_relay_address1",None),"WIN-QS4JOM4FNKK")
        self.assertEqual(policy_config.get_default("message_relay_id1",None),"e5fe429b-c478-42f1-aed4-6b891d21b5b8")

        self.assertEqual(policy_config.get_default("message_relay_priority2",None),"0")
        self.assertEqual(policy_config.get_default("message_relay_port2",None),"8190")
        self.assertEqual(policy_config.get_default("message_relay_address2",None),"LOSE-QS4JOM4FNKK")
        self.assertEqual(policy_config.get_default("message_relay_id2",None),"e5fethan-c478-42f1-alex-6b891d21b5b8")

        TEST_POLICY2="""<?xml version="1.0"?>
<policy type="mcs" xmlns:csc="com.sophos\msys\csc">
<configuration xmlns:auto-ns1="com.sophos\mansys\policy" xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="5" policyType="25"/>
    <messageRelays>
    </messageRelays>
</configuration>
</policy>"""
        command = FakePolicyCommand(TEST_POLICY2)
        adapter.process_command(command)
        self.assertTrue(command.m_complete)

        self.assertEqual(policy_config.get_default("message_relay_priority1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_port1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_address1",None),None)
        self.assertEqual(policy_config.get_default("message_relay_id1",None),None)

        self.assertEqual(policy_config.get_default("message_relay_priority2",None),None)
        self.assertEqual(policy_config.get_default("message_relay_port2",None),None)
        self.assertEqual(policy_config.get_default("message_relay_address2",None),None)
        self.assertEqual(policy_config.get_default("message_relay_id2",None),None)


if __name__ == '__main__':
    if "-q" in sys.argv:
        logging.disable(logging.CRITICAL)
    unittest.main()

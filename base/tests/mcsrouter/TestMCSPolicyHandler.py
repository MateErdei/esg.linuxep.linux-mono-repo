import logging
import sys
import unittest

import xml.dom.minidom
import PathManager

import mcsrouter.utils.config
import mcsrouter.adapters.mcs.mcs_policy_handler as policy_handler

import builtins

builtins.__dict__['REGISTER_MCS'] = False

MCS_POLICY_WITH_PROXY_AND_CREDS = """<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="0683e21443eb7d4d3e87b5b1887ecfe7d00ac584aa42745614b3793f3b283b30" policyType="25"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>mcstokenb44b0f0e7b56548f14f2854e87ea535f228a712506d2707d1c4f444</registrationToken>
    <customerId>9f42f1b9-2a49-e452-fa76-3105612b19a3</customerId>
    <servers>
      <server>https://mcs2-cloudstation.sophos.com/sophos/management/ep</server>
    </servers>
    <proxies>
      <proxy>http://192.168.36.37:3129</proxy>
    </proxies>
    <messageRelays/>
    <proxyCredentials>
      <credentials>Creds=part1/Creds=part2</credentials>
    </proxyCredentials>
    <useSystemProxy>true</useSystemProxy>
    <useAutomaticProxy>true</useAutomaticProxy>
    <useDirect>true</useDirect>
    <randomSkewFactor>1</randomSkewFactor>
    <commandPollingDelay default="20"/>
    <flagsPollingInterval default="14400"/>
    <policyChangeServers/>
    <presignedUrlService>
      <url>https://mcs2-cloudstation.sophos.com/sophos/management/ep/presignedurls</url>
      <credentials>CredsPart1/CredsPart2=</credentials>
    </presignedUrlService>
    <pushServers>
      <pushServer>https://mcs-push-server/ps</pushServer>
    </pushServers>
    <pushPingTimeout>60</pushPingTimeout>
    <pushFallbackPollInterval>90</pushFallbackPollInterval>
  </configuration>
</policy>
"""

MCS_REPLACEMENT_POLICY_WITH_PROXY_AND_CREDS = """<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="0683e21443eb7d4d3e87b5b1887ecfe7d00ac584aa42745614b3793f3b283b30" policyType="25"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>mcstokenb44b0f0e7b56548f14f2854e87ea535f228a712506d2707d1c4f444</registrationToken>
    <customerId>9f42f1b9-2a49-e452-fa76-3105612b19a3</customerId>
    <servers>
      <server>https://mcs2-cloudstation.sophos.com/sophos/management/ep</server>
    </servers>
    <proxies>
      <proxy>https_new://192.168.36.37:3130</proxy>
    </proxies>
    <messageRelays/>
    <proxyCredentials>
      <credentials>New_Creds=part12/New_Creds=part22</credentials>
    </proxyCredentials>
    <useSystemProxy>true</useSystemProxy>
    <useAutomaticProxy>true</useAutomaticProxy>
    <useDirect>true</useDirect>
    <randomSkewFactor>1</randomSkewFactor>
    <commandPollingDelay default="20"/>
    <flagsPollingInterval default="14400"/>
    <policyChangeServers/>
    <presignedUrlService>
      <url>https://mcs2-cloudstation.sophos.com/sophos/management/ep/presignedurls</url>
      <credentials>CredsPart1/CredsPart2=</credentials>
    </presignedUrlService>
    <pushServers>
      <pushServer>https://mcs-push-server/ps</pushServer>
    </pushServers>
    <pushPingTimeout>60</pushPingTimeout>
    <pushFallbackPollInterval>90</pushFallbackPollInterval>
  </configuration>
</policy>
"""

MCS_POLICY_WITH_PROXY_EMPTY_CREDS = """<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="0683e21443eb7d4d3e87b5b1887ecfe7d00ac584aa42745614b3793f3b283b30" policyType="25"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>mcstokenb44b0f0e7b56548f14f2854e87ea535f228a712506d2707d1c4f444</registrationToken>
    <customerId>9f42f1b9-2a49-e452-fa76-3105612b19a3</customerId>
    <servers>
      <server>https://mcs2-cloudstation.sophos.com/sophos/management/ep</server>
    </servers>
    <proxies>
      <proxy>http://192.168.36.37:3129</proxy>
    </proxies>
    <messageRelays/>
    <useSystemProxy>true</useSystemProxy>
    <useAutomaticProxy>true</useAutomaticProxy>
    <useDirect>true</useDirect>
    <randomSkewFactor>1</randomSkewFactor>
    <commandPollingDelay default="20"/>
    <flagsPollingInterval default="14400"/>
    <policyChangeServers/>
    <presignedUrlService>
      <url>https://mcs2-cloudstation.sophos.com/sophos/management/ep/presignedurls</url>
      <credentials>CredsPart1/CredsPart2=</credentials>
    </presignedUrlService>
    <pushServers>
      <pushServer>https://mcs-push-server/ps</pushServer>
    </pushServers>
    <pushPingTimeout>60</pushPingTimeout>
    <pushFallbackPollInterval>90</pushFallbackPollInterval>
  </configuration>
</policy>
"""

MCS_POLICY_NO_PROXY = """<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="0683e21443eb7d4d3e87b5b1887ecfe7d00ac584aa42745614b3793f3b283b30" policyType="25"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>mcstokenb44b0f0e7b56548f14f2854e87ea535f228a712506d2707d1c4f444</registrationToken>
    <customerId>9f42f1b9-2a49-e452-fa76-3105612b19a3</customerId>
    <servers>
      <server>https://mcs2-cloudstation.sophos.com/sophos/management/ep</server>
    </servers>
    <proxies/>
    <messageRelays/>
    <useSystemProxy>true</useSystemProxy>
    <useAutomaticProxy>true</useAutomaticProxy>
    <useDirect>true</useDirect>
    <randomSkewFactor>1</randomSkewFactor>
    <commandPollingDelay default="20"/>
    <flagsPollingInterval default="14400"/>
    <policyChangeServers/>
    <presignedUrlService>
      <url>https://mcs2-cloudstation.sophos.com/sophos/management/ep/presignedurls</url>
      <credentials>CredsPart1/CredsPart2=</credentials>
    </presignedUrlService>
    <pushServers>
    </pushServers>
    <pushPingTimeout>60</pushPingTimeout>
    <pushFallbackPollInterval>90</pushFallbackPollInterval>
  </configuration>
</policy>
"""

class TestMCSPolicyHandler(unittest.TestCase):

    def test_handler_process_policy_will_add_new_proxy_info_and_remove_old_proxy_info(self):
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        handler = policy_handler.MCSPolicyHandler(
            "install", policy_config, policy_config)

        handler.process(MCS_POLICY_WITH_PROXY_AND_CREDS)

        #verify credentials are populated at the beginning
        self.assertEqual(policy_config.get_default("pushFallbackPollInterval",None),'90')
        self.assertEqual(policy_config.get_default("COMMAND_CHECK_INTERVAL_MINIMUM",None),'20')
        self.assertEqual(policy_config.get_default("COMMAND_CHECK_INTERVAL_MAXIMUM",None),'20')
        self.assertEqual(policy_config.get_default("PUSH_SERVER_CHECK_INTERVAL",None),'90')
        self.assertEqual(policy_config.get_default("PUSH_SERVER_CONNECTION_TIMEOUT",None),'60')
        self.assertEqual(policy_config.get_default("MCSToken",None),"mcstokenb44b0f0e7b56548f14f2854e87ea535f228a712506d2707d1c4f444")
        self.assertEqual(policy_config.get_default("mcs_policy_url1",None),"https://mcs2-cloudstation.sophos.com/sophos/management/ep")
        self.assertEqual(policy_config.get_default("pushServer1",None),"https://mcs-push-server/ps")
        self.assertEqual(policy_config.get_default("mcs_policy_proxy",None),"http://192.168.36.37:3129")
        self.assertEqual(policy_config.get_default("mcs_policy_proxy_credentials",None),"Creds=part1/Creds=part2")

        handler.process(MCS_REPLACEMENT_POLICY_WITH_PROXY_AND_CREDS)

        self.assertEqual(policy_config.get_default("mcs_policy_proxy",None),"https_new://192.168.36.37:3130")
        self.assertEqual(policy_config.get_default("mcs_policy_proxy_credentials",None),"New_Creds=part12/New_Creds=part22")

    def test_handler_process_policy_will_remove_proxy_credentials_on_policy_change(self):
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        handler = policy_handler.MCSPolicyHandler(
            "install", policy_config, policy_config)

        handler.process(MCS_POLICY_WITH_PROXY_AND_CREDS)

        self.assertEqual(policy_config.get_default("mcs_policy_proxy",None),"http://192.168.36.37:3129")
        self.assertEqual(policy_config.get_default("mcs_policy_proxy_credentials",None),"Creds=part1/Creds=part2")

        handler.process(MCS_POLICY_WITH_PROXY_EMPTY_CREDS)

        self.assertEqual(policy_config.get_default("mcs_policy_url1",None),"https://mcs2-cloudstation.sophos.com/sophos/management/ep")
        self.assertEqual(policy_config.get_default("pushServer1",None),"https://mcs-push-server/ps")
        self.assertEqual(policy_config.get_default("mcs_policy_proxy",None),"http://192.168.36.37:3129")
        self.assertEqual(policy_config.get_default("mcs_policy_proxy_credentials",None),None)

    def test_handler_process_policy_will_remove_any_keys_not_in_new_policy(self):
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy.config")

        handler = policy_handler.MCSPolicyHandler(
            "install", policy_config, policy_config)

        handler.process(MCS_POLICY_WITH_PROXY_AND_CREDS)

        self.assertEqual(policy_config.get_default("pushServer1",None),"https://mcs-push-server/ps")
        self.assertEqual(policy_config.get_default("mcs_policy_proxy",None),"http://192.168.36.37:3129")
        self.assertEqual(policy_config.get_default("mcs_policy_proxy_credentials",None),"Creds=part1/Creds=part2")

        handler.process(MCS_POLICY_NO_PROXY)

        self.assertEqual(policy_config.get_default("mcs_policy_url1",None),"https://mcs2-cloudstation.sophos.com/sophos/management/ep")
        self.assertEqual(policy_config.get_default("pushServer1",None), None)
        self.assertEqual(policy_config.get_default("mcs_policy_proxy",None),None)
        self.assertEqual(policy_config.get_default("mcs_policy_proxy_credentials",None),None)

if __name__ == '__main__':
    if "-q" in sys.argv:
        logging.disable(logging.CRITICAL)
    unittest.main()

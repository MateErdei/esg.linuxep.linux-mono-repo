import logging
import sys
import unittest

import PathManager

import mcsrouter.utils.config
from mcsrouter.utils.default_values import *
import mcsrouter.adapters.mcs.mcs_policy_handler as policy_handler

import builtins

builtins.__dict__['REGISTER_MCS'] = False

MCS_POLICY_WITH_PROXY_AND_CREDS = """<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="0683e21443eb7d4d3e87b5b1887ecfe7d00ac584aa42745614b3793f3b283b30" policyType="25"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>mcstokenb44b0f0e7b56548f14f2854e87ea535f228a712506d2707d1c4f444</registrationToken>
    <customerId>9f42f1b9-2a49-e452-fa76-3105612b19a3</customerId>
    <tenantId>b4c43aab-1c44-4474-0805-c6697a01c411</tenantId>
    <deviceId>ba36e430-8356-4cdd-963c-819d293f5f48</deviceId>
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
    <tenantId>b4c43aab-1c44-4474-0805-c6697a01c411</tenantId>
    <deviceId>ba36e430-8356-4cdd-963c-819d293f5f48</deviceId>
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
    <tenantId>b4c43aab-1c44-4474-0805-c6697a01c411</tenantId>
    <deviceId>ba36e430-8356-4cdd-963c-819d293f5f48</deviceId>
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
    <tenantId>b4c43aab-1c44-4474-0805-c6697a01c411</tenantId>
    <deviceId>ba36e430-8356-4cdd-963c-819d293f5f48</deviceId>
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

# Missing fields - pushServer, commandPollingDelay, pushPingTimeout, flagsPollingInterval, pushPollFallbackInterval,
MCS_POLICY_WITH_MISSING_FIELDS = """<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="0683e21443eb7d4d3e87b5b1887ecfe7d00ac584aa42745614b3793f3b283b30" policyType="25"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>mcstokenb44b0f0e7b56548f14f2854e87ea535f228a712506d2707d1c4f444</registrationToken>
    <customerId>9f42f1b9-2a49-e452-fa76-3105612b19a3</customerId>
    <tenantId>b4c43aab-1c44-4474-0805-c6697a01c411</tenantId>
    <deviceId>ba36e430-8356-4cdd-963c-819d293f5f48</deviceId>
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
    <policyChangeServers/>
    <presignedUrlService>
      <url>https://mcs2-cloudstation.sophos.com/sophos/management/ep/presignedurls</url>
      <credentials>CredsPart1/CredsPart2=</credentials>
    </presignedUrlService>
    <pushServers>
    </pushServers>
  </configuration>
</policy>
"""

# Repeated fields - pushServers, commandPollingDelay, pushPingTimeout, flagsPollingInterval, pushPollFallbackInterval,
MCS_POLICY_WITH_MULTIPLE_FIELDS = """<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="0683e21443eb7d4d3e87b5b1887ecfe7d00ac584aa42745614b3793f3b283b30" policyType="25"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>mcstokenb44b0f0e7b56548f14f2854e87ea535f228a712506d2707d1c4f444</registrationToken>
    <customerId>9f42f1b9-2a49-e452-fa76-3105612b19a3</customerId>
    <tenantId>b4c43aab-1c44-4474-0805-c6697a01c411</tenantId>
    <deviceId>ba36e430-8356-4cdd-963c-819d293f5f48</deviceId>
    <servers>
      <server>https://mcs2-cloudstation.sophos.com/sophos/management/ep</server>
    </servers>
    <proxies/>
    <messageRelays/>
    <useSystemProxy>true</useSystemProxy>
    <useAutomaticProxy>true</useAutomaticProxy>
    <useDirect>true</useDirect>
    <randomSkewFactor>1</randomSkewFactor>
    <commandPollingDelay default="1"/>
    <commandPollingDelay default="2"/>
    <flagsPollingInterval default="3"/>
    <flagsPollingInterval default="4"/>
    <policyChangeServers/>
    <presignedUrlService>
      <url>https://mcs2-cloudstation.sophos.com/sophos/management/ep/presignedurls</url>
      <credentials>CredsPart1/CredsPart2=</credentials>
    </presignedUrlService>
    <pushServers>
      <pushServer>https://mcs-push-server/ps2</pushServer>
    </pushServers>
    <pushServers>
      <pushServer>https://mcs-push-server/ps1</pushServer>
    </pushServers>
    <pushPingTimeout>5</pushPingTimeout>
    <pushPingTimeout>6</pushPingTimeout>
    <pushFallbackPollInterval>7</pushFallbackPollInterval>
    <pushFallbackPollInterval>8</pushFallbackPollInterval>
  </configuration>
</policy>
"""

MCS_POLICY_WITH_INVALID_CHARACTERS = """<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="0683e21443eb7d4d3e87b5b1887ecfe7d00ac584aa42745614b3793f3b283b30" policyType="25"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>mcstokenb44b0f0e7b56548f14f2854e87ea535f228a712506d2707d1c4f444</registrationToken>
    <customerId>9f42f1b9-2a49-e452-fa76-3105612b19a3</customerId>
    <tenantId>b4c43aab-1c44-4474-0805-c6697a01c411</tenantId>
    <deviceId>ba36e430-8356-4cdd-963c-819d293f5f48</deviceId>
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
    <commandPollingDelay default="20a"/>
    <flagsPollingInterval default="14400a"/>
    <policyChangeServers/>
    <presignedUrlService>
      <url>https://mcs2-cloudstation.sophos.com/sophos/management/ep/presignedurls</url>
      <credentials>CredsPart1/CredsPart2=</credentials>
    </presignedUrlService>
    <pushServers>
      <pushServer>https://mcs-push-server/ps</pushServer>
    </pushServers>
    <pushPingTimeout>60a</pushPingTimeout>
    <pushFallbackPollInterval>90a</pushFallbackPollInterval>
  </configuration>
</policy>
"""

MCS_POLICY_WITH_VERY_LONG_VALUES = """<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="0683e21443eb7d4d3e87b5b1887ecfe7d00ac584aa42745614b3793f3b283b30" policyType="25"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>mcstokenb44b0f0e7b56548f14f2854e87ea535f228a712506d2707d1c4f444</registrationToken>
    <customerId>9f42f1b9-2a49-e452-fa76-3105612b19a3</customerId>
    <tenantId>b4c43aab-1c44-4474-0805-c6697a01c411</tenantId>
    <deviceId>ba36e430-8356-4cdd-963c-819d293f5f48</deviceId>
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
    <commandPollingDelay default="999999999999999999999999999999999999"/>
    <flagsPollingInterval default="999999999999999999999999999999999999"/>
    <policyChangeServers/>
    <presignedUrlService>
      <url>https://mcs2-cloudstation.sophos.com/sophos/management/ep/presignedurls</url>
      <credentials>CredsPart1/CredsPart2=</credentials>
    </presignedUrlService>
    <pushServers>
      <pushServer>https://mcs-push-server/ps</pushServer>
    </pushServers>
    <pushPingTimeout>999999999999999999999999999999999999</pushPingTimeout>
    <pushFallbackPollInterval>999999999999999999999999999999999999</pushFallbackPollInterval>
  </configuration>
</policy>
"""

MCS_POLICY_WITH_JSON_XML_INCORRECTLY = """<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="0683e21443eb7d4d3e87b5b1887ecfe7d00ac584aa42745614b3793f3b283b30" policyType="25"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>mcstokenb44b0f0e7b56548f14f2854e87ea535f228a712506d2707d1c4f444</registrationToken>
    <customerId>9f42f1b9-2a49-e452-fa76-3105612b19a3</customerId>
    <tenantId>b4c43aab-1c44-4474-0805-c6697a01c411</tenantId>
    <deviceId>ba36e430-8356-4cdd-963c-819d293f5f48</deviceId>
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
    <commandPollingDelay default="{'value': '20'}"/>
    <flagsPollingInterval default="<value>20</value>"/>
    <policyChangeServers/>
    <presignedUrlService>
      <url>https://mcs2-cloudstation.sophos.com/sophos/management/ep/presignedurls</url>
      <credentials>CredsPart1/CredsPart2=</credentials>
    </presignedUrlService>
    <pushServers>
      <pushServer>https://mcs-push-server/ps</pushServer>
    </pushServers>
    <pushPingTimeout>{'value': '20'}</pushPingTimeout>
    <pushFallbackPollInterval><value>20</value></pushFallbackPollInterval>
  </configuration>
</policy>
"""

MCS_POLICY_WITH_NEGATIVE_STRING_NUMBERS = """<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  <meta protocolVersion="1.1"/>
  <csc:Comp RevID="0683e21443eb7d4d3e87b5b1887ecfe7d00ac584aa42745614b3793f3b283b30" policyType="25"/>
  <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>mcstokenb44b0f0e7b56548f14f2854e87ea535f228a712506d2707d1c4f444</registrationToken>
    <customerId>9f42f1b9-2a49-e452-fa76-3105612b19a3</customerId>
    <tenantId>b4c43aab-1c44-4474-0805-c6697a01c411</tenantId>
    <deviceId>ba36e430-8356-4cdd-963c-819d293f5f48</deviceId>
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
    <commandPollingDelay default="-20"/>
    <flagsPollingInterval default="-14400"/>
    <policyChangeServers/>
    <presignedUrlService>
      <url>https://mcs2-cloudstation.sophos.com/sophos/management/ep/presignedurls</url>
      <credentials>CredsPart1/CredsPart2=</credentials>
    </presignedUrlService>
    <pushServers>
      <pushServer>https://mcs-push-server/ps</pushServer>
    </pushServers>
    <pushPingTimeout>"60"</pushPingTimeout>
    <pushFallbackPollInterval>"90"</pushFallbackPollInterval>
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
        self.assertEqual(policy_config.get_default("pushFallbackPollInterval", None), '90')
        self.assertEqual(policy_config.get_default("commandPollingDelay", None), '20')
        # we use the default set in central documentation if we do not get the information from the policy
        self.assertEqual(policy_config.get_default("flagsPollingInterval", None), '14400')
        self.assertEqual(policy_config.get_default("pushFallbackPollInterval", None), '90')
        self.assertEqual(policy_config.get_default("pushPingTimeout", None), '60')
        self.assertEqual(policy_config.get_default("MCSToken", None), "mcstokenb44b0f0e7b56548f14f2854e87ea535f228a712506d2707d1c4f444")
        self.assertEqual(policy_config.get_default("mcs_policy_url1", None),"https://mcs2-cloudstation.sophos.com/sophos/management/ep")
        self.assertEqual(policy_config.get_default("pushServer1", None), "https://mcs-push-server/ps")
        self.assertEqual(policy_config.get_default("mcs_policy_proxy", None), "http://192.168.36.37:3129")
        self.assertEqual(policy_config.get_default("mcs_policy_proxy_credentials", None), "Creds=part1/Creds=part2")

        handler.process(MCS_REPLACEMENT_POLICY_WITH_PROXY_AND_CREDS)

        self.assertEqual(policy_config.get_default("mcs_policy_proxy", None), "https_new://192.168.36.37:3130")
        self.assertEqual(policy_config.get_default("mcs_policy_proxy_credentials", None), "New_Creds=part12/New_Creds=part22")

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

    def test_handler_process_policy_will_handle_missing_fields_in_policy(self):
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy_config")

        handler = policy_handler.MCSPolicyHandler(
            "install", policy_config, policy_config
        )

        handler.process(MCS_POLICY_WITH_MISSING_FIELDS)

        self.assertEqual(policy_config.get_default("pushServer1", None), None)
        self.assertEqual(policy_config.get_default("commandPollingDelay", None), str(get_default_command_poll()))
        self.assertEqual(policy_config.get_default("pushFallbackPollInterval", None), str(get_default_command_poll()))
        self.assertEqual(policy_config.get_default("flagsPollingInterval", None), str(get_default_flags_poll()))
        self.assertEqual(policy_config.get_default("pushPingTimeout", None), str(get_default_push_ping_timeout()))

    def test_handler_process_policy_will_handle_repeated_fields_in_policy(self):
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy_config")

        handler = policy_handler.MCSPolicyHandler(
            "install", policy_config, policy_config
        )

        handler.process(MCS_POLICY_WITH_MULTIPLE_FIELDS)

        self.assertEqual(policy_config.get_default("pushServer1", None), "https://mcs-push-server/ps2")
        self.assertEqual(policy_config.get_default("commandPollingDelay", None), str(get_default_command_poll()))
        self.assertEqual(policy_config.get_default("pushFallbackPollInterval", None), str(get_default_command_poll()))
        self.assertEqual(policy_config.get_default("flagsPollingInterval", None), str(get_default_flags_poll()))
        self.assertEqual(policy_config.get_default("pushPingTimeout", None), str(get_default_push_ping_timeout()))

    def test_handler_process_policy_will_handle_invalid_characters_in_policy(self):
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy_config")

        handler = policy_handler.MCSPolicyHandler(
            "install", policy_config, policy_config
        )

        handler.process(MCS_POLICY_WITH_VERY_LONG_VALUES)

        self.assertEqual(policy_config.get_default("commandPollingDelay", None), str(get_max_for_any_value()))
        self.assertEqual(policy_config.get_default("pushFallbackPollInterval", None), str(get_max_for_any_value()))
        self.assertEqual(policy_config.get_default("flagsPollingInterval", None), str(get_max_for_any_value()))
        self.assertEqual(policy_config.get_default("pushPingTimeout", None), str(get_max_for_any_value()))

    def test_handler_process_policy_will_handle_repeated_fields_in_policy(self):
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy_config")

        handler = policy_handler.MCSPolicyHandler(
            "install", policy_config, policy_config
        )

        with self.assertRaises(RuntimeError) as excinfo:
            handler.apply_policy(MCS_POLICY_WITH_JSON_XML_INCORRECTLY)
        self.assertEqual(str(excinfo.exception), "Invalid xml policy")

    def test_handler_process_policy_will_handle_negatives_and_strings_for_numbers_in_policy(self):
        policy_config = mcsrouter.utils.config.Config("base/etc/sophosspl/mcs_policy_config")

        handler = policy_handler.MCSPolicyHandler(
            "install", policy_config, policy_config
        )

        handler.process(MCS_POLICY_WITH_NEGATIVE_STRING_NUMBERS)

        self.assertEqual(policy_config.get_default("commandPollingDelay", None), str(get_default_command_poll()))
        self.assertEqual(policy_config.get_default("pushFallbackPollInterval", None), str(get_default_command_poll()))
        self.assertEqual(policy_config.get_default("flagsPollingInterval", None), str(get_default_flags_poll()))
        self.assertEqual(policy_config.get_default("pushPingTimeout", None), str(get_default_push_ping_timeout()))

if __name__ == '__main__':
    if "-q" in sys.argv:
        logging.disable(logging.CRITICAL)
    unittest.main()

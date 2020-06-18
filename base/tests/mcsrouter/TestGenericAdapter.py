import os
import unittest
import mock

import sys
import json
import builtins
import datetime

import logging
logger = logging.getLogger("TestMCS")

BUILD_DIR=os.environ.get("ABS_BUILDDIR",".")
INSTALL_DIR=os.path.join(BUILD_DIR,"install")

import PathManager

import mcsrouter.mcs
import mcsrouter.mcsclient.mcs_exception
import mcsrouter.mcsclient.mcs_connection
import mcsrouter.mcsclient.mcs_commands as mcs_commands
import mcsrouter.adapters.generic_adapter as generic_adapter
import mcsrouter.adapters.agent_adapter as agent_adapter
import mcsrouter.utils.target_system_manager

class FakeCommand(mcs_commands.PolicyCommand):
    def __init__(self, policy):
        mcs_commands.PolicyCommand.__init__(self, "cmd_id", "ALC", "p_id", None)
        self.__p = policy
    def complete(self):
        pass
    def get_policy(self):
        return self.__p
policyContent = """<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
    <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
    <AUConfig platform="Linux">
        <sophos_address address="http://es-web.sophos.com/update"/>
        <primary_location>
            <server BandwidthLimit="0" AutoDial="false" Algorithm="AES256" UserPassword="CCD37FNeOPt7oCSNouRhmb9TKqwDvVsqJXbyTn16EHuw6ksTa3NCk56J5RRoVigjd3E=" UserName="regruser" UseSophos="true" UseHttps="true" UseDelta="true" ConnectionAddress="http://dci.sophosupd.com/cloudupdate" AllowLocalConfig="false"/>
            <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
        </primary_location>
        <secondary_location>
            <server BandwidthLimit="0" AutoDial="false" Algorithm="" UserPassword="" UserName="" UseSophos="false" UseHttps="true" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
            <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
        </secondary_location>
        <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="20" DetectDialUp="false"/>

        <logging AllowLocalConfig="false" LogLevel="50" LogEnable="true" MaxLogFileSize="1"/>
        <bootstrap Location="" UsePrimaryServerAddress="true"/>
        <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
        <cloud_subscriptions>
            <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
            <subscription Id="MDR" RigidName="ServerProtectionLinux-Plugin-MDR" Tag="RECOMMENDED"/>
            <subscription Id="SENSORS" RigidName="ServerProtectionLinux-Plugin-AuditPlugin" Tag="RECOMMENDED"/>
            <subscription Id="SENSORS" RigidName="ServerProtectionLinux-Plugin-EventProcessor" Tag="RECOMMENDED"/>
        </cloud_subscriptions>
        <delay_supplements enabled="false"/>
    </AUConfig>
    <Features>
        <Feature id="CORE"/>
    </Features>
    <intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"/>
    <customer id="4b4ca3ba-c144-4447-8050-6c96a7104c11"/>
</AUConfigurations>
"""


class TestGenericAdapter(unittest.TestCase):

    def testGenericProcessCommand(self):

        m = generic_adapter.GenericAdapter('ALC', INSTALL_DIR)
        self.assertEqual(m.get_app_id(), 'ALC')
        alc_policy = FakeCommand(policyContent)
        mocked_open_function = mock.mock_open()
        with mock.patch("builtins.open", mocked_open_function):
            m.process_command(alc_policy)

    def testBrokenPolicyShouldNotCrashGenericAdapter(self):

        m = generic_adapter.GenericAdapter('ALC', INSTALL_DIR)
        self.assertEqual(m.get_app_id(), 'ALC')
        invalid_alc_policy="""<?xml version='1.0'?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="


" xmlns="http://www.sophos.com/EE/AUConfig">
<csc:Comp RevID="5514d7970027ac81cc3686799c9359043dafbc72d4b809490ca82bacc4bf5026" policyType="1"></csc:Comp>
<AUConfig platform="Linux">
<sophos_address address="http://es-web.sophos.com/update"></sophos_address>
<primary_location>
<server BandwidthLimit="256" AutoDial="false" Algorithm="Clear" UserPassword="xn28ddszs1q" UserName="CSP7I0S0GZZE" UseSophos="true" UseHttps="true" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"></server>
<proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"></proxy>
</primary_location>
<secondary_location>
<server BandwidthLimit="256" AutoDial="false" Algorithm="" UserPassword="" UserName="" UseSophos="false" UseHttps="true" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"></server>
<proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"></proxy>
</secondary_location>
<schedule AllowLocalConfig="false" SchedEnable="true" Frequency="60" DetectDialUp="false"></schedule>
<logging AllowLocalConfig="false" LogLevel="50" LogEnable="true" MaxLogFileSize="1"></logging>
<bootstrap Location="" UsePrimaryServerAddress="true"></bootstrap>
<cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"></cloud_subscription>
<cloud_subscriptions>
<subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"></subscription>
<subscription Id="MDR" RigidName="ServerProtectionLinux-Plugin-MDR" Tag="RECOMMENDED"></subscription>
</cloud_subscriptions>
<delay_supplements enabled="true"></delay_supplements>
</AUConfig>
<Features>
<Feature id="CORE"></Feature>
<Feature id="MDR"></Feature>
<Feature id="SDU"></Feature>
</Features>
<intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"></intelligent_updating>
<customer id="ad936fd6-329d-e43b-cb15-3635be1058db"></customer>
</AUConfigurations>"""
        alc_policy = FakeCommand(invalid_alc_policy)
        mocked_open_function = mock.mock_open()
        with mock.patch("builtins.open", mocked_open_function):
            m.process_command(alc_policy)



    def testGetStatus(self):
        m = generic_adapter.GenericAdapter('ALC', INSTALL_DIR)
        self.assertEqual(m.get_app_id(), 'ALC')
        mocked_open_function = mock.mock_open(read_data=policyContent.encode('utf-8'))
        with mock.patch("builtins.open", mocked_open_function):
            m.get_status_and_config_xml()

    @mock.patch('os.listdir', return_value=["ALC-1_policy.xml"])
    @mock.patch('os.path.isfile', return_value=True)
    @mock.patch('os.path.getmtime', return_value=1567677734.4998658)
    def testAgentAdapter(self, *mockargs):
        agent = agent_adapter.AgentAdapter(INSTALL_DIR)
        mocked_open_function = mock.mock_open(read_data="""{"policyAppIds":["ALC","MDR"]}""")
        with mock.patch("builtins.open", mocked_open_function):
            self.assertEqual(agent.get_policy_status(), """<policy-status latest="1970-01-01T00:00:00.0Z"><policy app="ALC" latest="2019-09-05T10:02:14.499865Z" /></policy-status>""")

    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
    def testComputerStatus(self, *mockargs):
        target_system = mcsrouter.utils.target_system_manager.get_target_system('/tmp/sophos-spl')
        status_xml = agent_adapter.ComputerCommonStatus(target_system).to_status_xml()
        self.assertNotIn("b'", status_xml)

    def test_convert_ttl_to_epoch_time_with_ttl_10000(self):
        adapter = generic_adapter.GenericAdapter('ALC', INSTALL_DIR)
        timestamp = "2020-06-09T15:30:08.415Z"
        ttl = 10000
        ttl_string = f"PT{ttl}S"
        epoch_time = adapter._convert_ttl_to_epoch_time(timestamp, ttl_string)
        self.assertEqual(epoch_time, 1591726608)

    def test_convert_ttl_to_epoch_time_with_ttl_20000(self):
        adapter = generic_adapter.GenericAdapter('ALC', INSTALL_DIR)
        timestamp = "2020-06-09T15:30:08.415Z"
        ttl = 20000
        ttl_string = f"PT{ttl}S"
        epoch_time = adapter._convert_ttl_to_epoch_time(timestamp, ttl_string)
        self.assertEqual(epoch_time, 1591736608)

    def test_convert_ttl_to_epoch_time_returns_if_invalid_ttl_format(self):
        adapter = generic_adapter.GenericAdapter('ALC', INSTALL_DIR)
        timestamp = "2020-06-09T15:30:08.415Z"
        ttl = 2000
        expected_ttl_epoch = 1591726608
        def test_invalid_ttl(ttl_string):
            epoch_time = adapter._convert_ttl_to_epoch_time(timestamp, ttl_string)
            self.assertEqual(expected_ttl_epoch, epoch_time)

        test_invalid_ttl(f"PT{ttl}SF")
        test_invalid_ttl(f"EPT{ttl}S")
        test_invalid_ttl(f"{ttl}")
        test_invalid_ttl(f"PTS")
        test_invalid_ttl(f"SD{ttl}P")
        test_invalid_ttl(f"completelyinvalid")

if __name__ == '__main__':
    import logging
    unittest.main()
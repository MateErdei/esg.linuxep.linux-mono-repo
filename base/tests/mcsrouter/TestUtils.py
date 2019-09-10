


import os
import unittest
import mock
import sys
import json
import builtins

import logging
logger = logging.getLogger("TestUtils")

BUILD_DIR=os.environ.get("ABS_BUILDDIR",".")
INSTALL_DIR=os.path.join(BUILD_DIR,"install")

import PathManager

import mcsrouter.utils.plugin_registry as plugin_registry
import mcsrouter.utils.xml_helper as xml_helper
import mcsrouter.mcsclient.status_event as status_event

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

class TestPluginRegistry(unittest.TestCase):
    @mock.patch('os.listdir', return_value=["dummyplugin.json"])
    def test_added_and_removed_app_ids(self, *mockargs):
        mocked_open_function = mock.mock_open(read_data="""{"policyAppIds":["ALC","MDR"]}""")

        with mock.patch("builtins.open", mocked_open_function):
            pr = plugin_registry.PluginRegistry(INSTALL_DIR)
            added, removed = pr.added_and_removed_app_ids()
            self.assertEqual( added, ['ALC', 'MDR'])
            self.assertEqual(removed, [])
        mocked_open_function = mock.mock_open(read_data="""{"policyAppIds":["MDR"]}""")

        with mock.patch("builtins.open", mocked_open_function):
            added, removed = pr.added_and_removed_app_ids()
            self.assertEqual( added, [])
            self.assertEqual(removed, ['ALC'])

class TestUtils(unittest.TestCase):
    def test_escaped_non_ascii_content(self):
        mocked_open_function = mock.mock_open(read_data=policyContent.encode('utf-8'))

        with mock.patch("builtins.open", mocked_open_function):
            content = xml_helper.get_escaped_non_ascii_content('thisfilepath.xml')
        self.assertTrue( 'Features' in content)
        self.assertEqual(type(content), type(''))

alc_status="""<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="@@revid@@" policyType="1" />
</status>"""
alc_expected="""<?xml version="1.0" encoding="utf-8"?><ns:statuses schemaVersion="1.0" xmlns:ns="http://www.sophos.com/xml/mcs/statuses"><status><appId>ALC</appId><creationTime>20190912T100000</creationTime><ttl>2</ttl><body>&lt;?xml version=&quot;1.0&quot; encoding=&quot;utf-8&quot; ?&gt;
&lt;status xmlns=&quot;com.sophos\mansys\status&quot; type=&quot;sau&quot;&gt;
    &lt;CompRes xmlns=&quot;com.sophos\msys\csc&quot; Res=&quot;Same&quot; RevID=&quot;@@revid@@&quot; policyType=&quot;1&quot; /&gt;
&lt;/status&gt;</body></status></ns:statuses>"""

class TestStatusEvents(unittest.TestCase):
    def testStatusEventXmlShouldReturnString(self):
        se = status_event.StatusEvent()
        se.add_adapter('ALC', '2', '20190912T100000', alc_status)
        self.assertEqual(se.xml(), alc_expected)




if __name__ == '__main__':
    import logging
    unittest.main()

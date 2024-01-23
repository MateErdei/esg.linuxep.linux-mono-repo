import logging
import os
import unittest

from unittest import mock

logger = logging.getLogger("TestUtils")

BUILD_DIR=os.environ.get("ABS_BUILDDIR",".")
INSTALL_DIR=os.path.join(BUILD_DIR,"install")
import PathManager
import tempfile
import shutil
import re
import mcsrouter.utils.plugin_registry as plugin_registry
import mcsrouter.utils.migration as migration
import mcsrouter.utils.xml_helper as xml_helper
import mcsrouter.utils.verification as verification
import mcsrouter.mcsclient.status_event as status_event
import mcsrouter.sophos_https as sophos_https
import mcsrouter.ip_selection as ip_selection
from mcsrouter import ip_address
from mcsrouter.mcs_router import SophosLogging
from mcsrouter.mcs_router import hostfile_has_read_permission

import xml.parsers.expat
import json.decoder

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


class TestMigration(unittest.TestCase):
    def testReadMigrationActionGetsURLAndToken(self):
        m = migration.Migrate()
        m.read_migrate_action("""<?xml version='1.0'?><action type="sophos.mgt.mcs.migrate"><server>http://es-web.sophos.com/update</server><token>jwt</token></action>""")
        self.assertEqual(m.get_migrate_url(), 'http://es-web.sophos.com/update')
        self.assertEqual(m.get_token(), 'jwt')

    def testReadMigrationActionGetsURLAndTokenForLongServer(self):
        name = "n" * 1000000
        long_server = """<?xml version='1.0'?><action type="sophos.mgt.mcs.migrate"><server>"""
        long_server += name
        long_server += """</server><token>jwt</token></action>"""
        m = migration.Migrate()
        m.read_migrate_action(long_server)
        self.assertEqual(m.get_migrate_url(), name)
        self.assertEqual(m.get_token(), 'jwt')

    def testReadMigrationActionRaisesExceptionForMissingServer(self):
        missing_server = """<?xml version='1.0'?><action type="sophos.mgt.mcs.migrate"><token>jwt</token></action>"""
        m = migration.Migrate()
        self.assertRaises(IndexError, m.read_migrate_action, missing_server)

    def testReadMigrationActionRaisesExceptionForMissingToken(self):
        missing_token = """<?xml version='1.0'?><action type="sophos.mgt.mcs.migrate"><server>http://es-web.sophos.com/update</server></action>"""
        m = migration.Migrate()
        self.assertRaises(IndexError, m.read_migrate_action, missing_token)

    def testReadMigrationActionRaisesExceptionForInvalidXML(self):
        m = migration.Migrate()
        self.assertRaises(xml.parsers.expat.ExpatError, m.read_migrate_action, 'some text')

    def testExtractValuesGetsIDs(self):
        m = migration.Migrate()
        endpoint_id, device_id, tenant_id, password = m.extract_values('{"endpoint_id":"endpoint","device_id":"device","tenant_id":"tenant","password":"foobar"}')
        self.assertEqual(endpoint_id, 'endpoint')
        self.assertEqual(device_id, 'device')
        self.assertEqual(tenant_id, 'tenant')
        self.assertEqual(password, 'foobar')

    def testExtractValuesWithMismatchedTypes(self):
        m = migration.Migrate()
        endpoint_id, device_id, tenant_id, password = m.extract_values('{"endpoint_id":0,"device_id":1,"tenant_id":true,"password":false}')
        self.assertEqual(endpoint_id, 0)
        self.assertEqual(device_id, 1)
        self.assertEqual(tenant_id, True)
        self.assertEqual(password, False)

    def testExtractValuesRaisesExceptionForInvalidJSON(self):
        m = migration.Migrate()
        self.assertRaises(json.decoder.JSONDecodeError, m.extract_values, 'some text')


class TestUtils(unittest.TestCase):
    def test_escaped_non_ascii_content(self):
        mocked_open_function = mock.mock_open(read_data=policyContent.encode('utf-8'))

        with mock.patch("builtins.open", mocked_open_function):
            content = xml_helper.get_escaped_non_ascii_content('thisfilepath.xml')
        self.assertTrue( 'Features' in content)
        self.assertEqual(type(content), type(''))

    def test_parseString(self):
        passed = False
        try:
            xml_helper.parseString(b'')
        except xml.parsers.expat.ExpatError:
            passed = True
        self.assertTrue(passed)

        alc_policy = xml_helper.parseString(policyContent)

        elements = alc_policy.getElementsByTagName('subscription')
        self.assertEqual( len(elements), 4)


    def test_parseString(self):
        """Parse xml will produce utf-8 strings if the input is an utf-8 string or bytearray"""
        parsed_string="""<?xml version="1.0" ?><hello/>"""
        string_input = """<?xml version="1.0" encoding="utf-8" ?><hello/>"""
        bytes_string_input = b"""<?xml version="1.0" encoding="utf-8" ?><hello/>"""
        for input_s in (string_input, bytes_string_input):
            doc = xml_helper.parseString(input_s)
            self.assertTrue(isinstance(doc, xml.dom.minidom.Document))
            s = doc.toxml()
            self.assertTrue(isinstance(s, str))
            self.assertEqual(s, parsed_string)

    def test_get_escaped_non_ascii_content(self):
        fd, filepath = tempfile.mkstemp()
        try:
            os.write(fd, b"""<?xml version="1.0" ?><hello/>""")
            os.close(fd)
            content = xml_helper.get_escaped_non_ascii_content(filepath)
            self.assertTrue(isinstance(content, str))
        finally:

            os.remove(filepath)
    @mock.patch("os.access", return_value=False)
    def test_hostfile_has_read_permissions(self, *mockargs):
        assert(hostfile_has_read_permission() == False)






alc_status="""<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="@@revid@@" policyType="1" />
</status>"""

class TestStatusEvents(unittest.TestCase):
    def testStatusEventXmlShouldReturnExpectedString(self):
        """
        Expected XML for reference:
        <?xml version="1.0" encoding="utf-8"?><ns:statuses xmlns:ns="http://www.sophos.com/xml/mcs/statuses" schemaVersion="1.0">
        <status>
            <appId>ALC</appId>
            <creationTime>20190912T100000</creationTime>
            <ttl>2</ttl>
            <body>
                <?xml version="1.0" encoding="utf-8" ?><status xmlns="com.sophos\mansys\status" type="sau">
                    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="@@revid@@" policyType="1" />
                </status>
            </body>
        </status></ns:statuses>
        """
        se = status_event.StatusEvent()
        se.add_adapter('ALC', '2', '20190912T100000', alc_status)
        self.assertEqual(type(se.xml()), str)

        seAsXml = xml_helper.parseString(se.xml())

        self.assertEqual(seAsXml.documentElement.tagName, "ns:statuses")
        self.assertEqual(seAsXml.documentElement.getAttribute("xmlns:ns"), "http://www.sophos.com/xml/mcs/statuses")
        self.assertEqual(seAsXml.documentElement.getAttribute("schemaVersion"), "1.0")

        self.assertEqual(seAsXml.getElementsByTagName("status")[0].childNodes.length, 4)

        appId = seAsXml.getElementsByTagName("status")[0].firstChild
        self.assertEqual(appId.nodeName, "appId")
        self.assertEqual(appId.firstChild.nodeValue, "ALC")

        creationTime = appId.nextSibling
        self.assertEqual(creationTime.nodeName, "creationTime")
        self.assertEqual(creationTime.firstChild.nodeValue, "20190912T100000")

        ttl = creationTime.nextSibling
        self.assertEqual(ttl.nodeName, "ttl")
        self.assertEqual(ttl.firstChild.nodeValue, "2")

        body = ttl.nextSibling
        expectedBody = """<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="@@revid@@" policyType="1" />
</status>"""
        self.assertEqual(body.nodeName, "body")
        self.assertEqual(body.firstChild.nodeValue, expectedBody)


class TestSophosHTTPS(unittest.TestCase):
    def testProxyProducesHeaderAsString(self):
        proxy = sophos_https.Proxy()
        proxy.m_username = 'user'
        proxy.m_password = 'pass'
        self.assertEqual(proxy.auth_header(), 'Basic dXNlcjpwYXNz')

class TestIPSelection(unittest.TestCase):
    def test_ip_address_distance(self):
        self.assertEqual(ip_selection.get_ip_address_distance('127.0.0.1', '31.222.175.174'), 31)
        self.assertEqual(ip_selection.get_ip_address_distance('127.0.1.1', '127.1.3.3'), 17)
        self.assertEqual(ip_selection.get_ip_address_distance('127.0.1.5', '127.0.1.3'), 3)
        self.assertEqual(ip_selection.get_ip_address_distance('127.0.1.3', '127.0.1.3'), 0)
        self.assertEqual(ip_selection.get_ip_address_distance('127.0.1.1', '127.0.1.2'), 2)
        self.assertEqual(ip_selection.get_ip_address_distance('10.0.2.15', '10.0.2.14'), 1)
        self.assertEqual(ip_selection.get_ip_address_distance('10.0.2.15', '10.0.2.31'), 5)
        self.assertEqual(ip_selection.get_ip_address_distance('10.0.2.15', '10.0.3.15'), 9)
        self.assertEqual(ip_selection.get_ip_address_distance('10.0.2.15', '11.0.2.15'), 25)

    def test_order_by_order_by_ip_address_distance(self):
        local6s = []
        local4s = ['10.0.2.15']
        servers_list = ['10.0.2.14', '10.0.2.31', '10.0.3.15', '11.0.2.15']
        copy_servers = servers_list[:]
        copy_servers.reverse()
        servers = [{'ips': [ip]} for ip in copy_servers]

        ans = ip_selection.order_by_ip_address_distance(local4s, local6s, servers)
        self.assertEqual(ans[0], {'ips': ['10.0.2.14'], 'dist': 1})
        self.assertEqual(ans[1], {'ips': ['10.0.2.31'], 'dist': 5})
        self.assertEqual(ans[2], {'ips': ['10.0.3.15'], 'dist': 9})
        self.assertEqual(ans[3], {'ips': ['11.0.2.15'], 'dist': 25})

    def test_alg_evaluate_address_preference(self):
        local_ipv4s= ['10.0.2.15']
        local_ipv6s= [338288524927261089654739516762171489380]
        server_location_list=[
            {'hostname': 'FakeRelayTwentyFive', 'port': '6666', 'priority': '0', 'id': '1', 'ips': ['11.0.2.15']},
            {'hostname': 'FakeRelayFive', 'port': '6666', 'priority': '0', 'id': '2', 'ips': ['10.0.2.31']},
            {'hostname': 'FakeRelayNine', 'port': '6666', 'priority': '0', 'id': '4', 'ips': ['10.0.3.15']},
            {'hostname': 'ssplsecureproxyserver.eng.sophos', 'port': '8888', 'priority': '1', 'id': '5', 'ips': ['10.55.36.245']}
        ]
        server_location_list = ip_selection.order_message_relays(server_location_list, local_ipv4s, local_ipv6s)
        hostnames=['FakeRelayFive', 'FakeRelayNine', 'FakeRelayTwentyFive', 'ssplsecureproxyserver.eng.sophos' ]
        self.assertEqual(len(server_location_list), 4)
        hosts = [l['hostname'] for l in server_location_list]
        self.assertEqual(hostnames, hosts)

    @mock.patch("socket.getaddrinfo", return_value=Exception)
    @mock.patch("logging.Logger.warning")
    def test_host_name_logging_messagerelays(self, *mockargs):
        server_location_list = [
            {'hostname': 'FakeRelayTwentyFive', 'port': '6666', 'priority': '0', 'id': '1', 'ips': ['11.0.2.15']},
            {'hostname': 'FakeRelayFive', 'port': '6666', 'priority': '0', 'id': '2', 'ips': ['10.0.2.31']}
        ]
        ip_selection.evaluate_address_preference(server_location_list)
        calls = [mock.call("Extracting ip from server FakeRelayTwentyFive resulted in exception 'type' object is not iterable"),
                 mock.call("Extracting ip from server FakeRelayFive resulted in exception 'type' object is not iterable")]
        logging.Logger.warning.assert_has_calls(calls)

    def test_ip_fqdn(self):
        fq = ip_address.get_fqdn()
        self.assertNotIn("b'", fq)

class TestSophosLogging(unittest.TestCase):

    def test_sophos_logging_does_not_crash_application(self, *mockargs):
        dirn=tempfile.mkdtemp()
        try:
            session_dup= '''[sophos_managementagent]\nVERBOSITY=DEBUG\n[sophos_managementagent]\nVERBOSITY=INFO\n'''
            loggerDir=os.path.join(dirn, 'base/etc')
            os.makedirs(loggerDir, exist_ok=True)
            os.makedirs(os.path.join(dirn, 'logs/base/sophosspl'), exist_ok=True)

            with open(os.path.join(loggerDir, 'logger.conf'), 'w') as f:
                f.write(session_dup)
            sl=SophosLogging(dirn)
            log= logging.Logger('test')
            log.info('test')
            sl.shutdown()
        finally:
            shutil.rmtree(dirn)

class TestVerification(unittest.TestCase):
    def test_invalid_urls(self, *mockargs):
        self.assertRaises(verification.MCSUrlVerificationException, verification.check_url, "")
        self.assertRaises(verification.MCSUrlVerificationException, verification.check_url, "http://hostname//")
        self.assertRaises(verification.MCSUrlVerificationException, verification.check_url, "http://!hostname/")
        self.assertRaises(verification.MCSUrlVerificationException, verification.check_url, "http://hostname:iop/")
        self.assertRaises(verification.MCSUrlVerificationException, verification.check_url, "http://http://")
        self.assertRaises(verification.MCSUrlVerificationException, verification.check_url, "http://::")
        self.assertRaises(verification.MCSUrlVerificationException, verification.check_url, ":::")

    def test_valid_urls(self, *mockargs):
        try:
            verification.check_url("http://hostname/")
            verification.check_url("http://hostname:8080/")
            verification.check_url("https://hostname/")
            verification.check_url("https://hostname:8080/")
        except verification.MCSUrlVerificationException as exception:
            self.fail(f"check_url() raised error {exception}")
def assert_message_in_logs(message, logArray, log_level=None):
    for log_message in logArray:
        if message in log_message:
            if log_level:
                if re.match(r'^{}:'.format(log_level), log_message):
                    return
                else:
                    raise AssertionError( "Messsage: {}, not found in {} logs: {}".format(message, log_level, logArray))
            return
    raise AssertionError( "Messsage: {}, not found in logs: {}".format(message, logArray))


def assert_message_not_in_logs(message, log_array):
    for log_message in log_array:
        if message in log_message:
            raise AssertionError("Messsage: {}, found in logs: {}".format(message, log_array))


if __name__ == '__main__':
    import logging
    unittest.main()


import unittest
import logging
from unittest import mock
import time
import selectors
import sseclient
import PathManager
import mcsrouter.utils.config
import mcsrouter.sophos_https as sophos_https
from types import SimpleNamespace
from mcsrouter.mcs_push_client import MCSPushClientInternal, PipeChannel, MCSPushClient, MCSPushException, MCSPushSetting, PushClientStatus


DIRECT_CONNECTION = sophos_https.Proxy()

class ExtendedPipeChannel(PipeChannel):
    def __init__(self):
        PipeChannel.__init__(self)

    def fileno(self):
        return self.read

class FakeSSEClient:
    def __init__(self):
        self.resp = SimpleNamespace()
        self.resp.raw = ExtendedPipeChannel()
        self._msg = None

    def push_message(self, msg):
        msg = SimpleNamespace(data=msg)
        self._msg = msg

    def __next__(self):
        if self._msg:
            copy_msg = self._msg
            self.resp.raw.clear()
            self._msg = None
            return copy_msg
        raise StopIteration()

    def clear(self):
        self.resp.raw = ExtendedPipeChannel()

GlobalFakeSSeClient = FakeSSEClient()

class FakeSSEClientFactory:
    def __init__(self):
        global GlobalFakeSSeClient
        GlobalFakeSSeClient.clear()

    def __call__(self, *args, **kwargs):
        global GlobalFakeSSeClient
        return GlobalFakeSSeClient

class SSEClientSimulateConnectionFailureFactory:
    def __init__(self):
        pass

    def __call__(self, *args, **kwargs):
        raise RuntimeError("Failed to connect")


class SharedTestsUtilities(unittest.TestCase):
    def get_events_from_pipe(self, pipe_input):
        sel = selectors.DefaultSelector()
        sel.register(pipe_input, selectors.EVENT_READ, 'anything')
        return sel.select(timeout=0.3)

    def send_and_receive_message(self, msg, push_client):
        global GlobalFakeSSeClient
        GlobalFakeSSeClient.push_message(msg)
        GlobalFakeSSeClient.resp.raw.notify()
        events = self.get_events_from_pipe(push_client.notify_activity_pipe())
        self.assertEqual(len(events), 1, msg="events: {}".format(events))
        pending = push_client.pending_commands()
        self.assertEqual(len(pending), 1, msg="events: {}".format(pending))
        return pending[0].msg

class TestMCSPushClientInternal(SharedTestsUtilities):

    def setUp(self) -> None:
        self.client_internal = None


    def get_client(self, proxies=[DIRECT_CONNECTION]):
        header = {
            "User-Agent": "Sophos MCS Client/0.1 x86_64 sessions regToke",
            "Authorization": f"Bearer <dummy_jwt_token>",
            "Content-Type": "application/json",
            "X-Device-ID": "thisisadeviceid",
            "X-Tenant-ID": "thisisatenantid",
            "Accept": "application/json",
        }
        sett= MCSPushSetting(url="url", cert="cert", expected_ping=10,
                             proxy_settings=proxies, connection_header=header,
                             device_id="thisisadeviceid", tenant_id="thisisatenantid")
        self.client_internal = MCSPushClientInternal(sett)
        return self.client_internal

    def tearDown(self) -> None:
        if self.client_internal:
            self.client_internal.stop()


    def test_pipe_channel_can_notify_and_clear(self):
        p = PipeChannel()
        p.notify()
        sel = selectors.DefaultSelector()
        sel.register(p.read, selectors.EVENT_READ, 'anything')
        self.assertEqual(len(sel.select(timeout=0.3)), 1)
        p.clear()
        self.assertEqual(len(sel.select(timeout=0.3)), 0)


    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_can_start_stop(self, *mockargs):
        client = self.get_client()
        client.start()
        time.sleep(0.3)
        client.stop()

    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_can_receive_and_process_messages(self, *mockargs):
        client = self.get_client()
        client.start()
        received_msg = self.send_and_receive_message('hello', client)
        self.assertEqual(received_msg, 'hello')
        client.stop()

    @mock.patch("logging.Logger.debug")
    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_pings_are_logged(self, *mockargs):
        client = self.get_client()
        client.start()
        global GlobalFakeSSeClient
        GlobalFakeSSeClient.push_message('')
        GlobalFakeSSeClient.resp.raw.notify()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Server sent ping"))
        pending = client.pending_commands()
        self.assertEqual(len(pending), 0)

class ConfigWithoutFile( mcsrouter.utils.config.Config):
    def load(self, filename):
        pass
    def __init__(self):
        mcsrouter.utils.config.Config.__init__(self)
        self.set('pushServer1', 'value')
        self.set('PUSH_SERVER_CONNECTION_TIMEOUT', '10')
        self.set('device_id', 'thisisadeviceid')
        self.set('tenant_id', 'thisisatenantid')

class TestMCSPushConfigSettings(unittest.TestCase):
    def default_push_settings_args(self):
        proxy = [sophos_https.Proxy("http://localhost", 4335, "username", "password")]
        header = {
            "User-Agent": "Sophos MCS Client/0.1 x86_64 sessions regToke",
            "Authorization": f"Bearer <dummy_jwt_token>",
            "Content-Type": "application/json",
            "X-Device-ID": "thisisadeviceid",
            "X-Tenant-ID": "thisisatenantid",
            "Accept": "application/json",
        }
        return ['certpath', proxy, header]

    def test_extract_values_from_config(self):
        settings = MCSPushSetting.from_config(ConfigWithoutFile(), *self.default_push_settings_args())
        self.assertEqual(settings.url, 'value/v2/push/device/thisisadeviceid')
        self.assertEqual(settings.expected_ping,  10)
        self.assertEqual(settings.cert, 'certpath')
        self.assertEqual(settings.proxy_settings[0].host(), 'localhost')
        self.assertEqual(settings.proxy_settings[0].port(), 4335)
        self.assertEqual(settings.proxy_settings[0].username(), "username")
        self.assertEqual(settings.proxy_settings[0].password(), "password")

    def test_settings_equality(self):
        settings = MCSPushSetting.from_config(ConfigWithoutFile(), *self.default_push_settings_args())
        settings1 = MCSPushSetting.from_config(ConfigWithoutFile(), *self.default_push_settings_args())
        self.assertEqual(settings, settings1)
        settings1.url = 'changed'
        self.assertNotEqual(settings, settings1)

        proxy2 = [sophos_https.Proxy("http://localhost", 4335, "username", "changed_password")]
        settings2 = MCSPushSetting.from_config(ConfigWithoutFile(), *self.default_push_settings_args())
        settings3 = MCSPushSetting.from_config(ConfigWithoutFile(), 'certpath', proxy2, ('user', 'pass'))
        self.assertEqual(settings, settings2)
        self.assertNotEqual(settings, settings3)

    def test_change_authorization_is_detected_as_settings_changed(self):
        settings = MCSPushSetting.from_config(ConfigWithoutFile(), *self.default_push_settings_args())
        settings1 = MCSPushSetting.from_config(ConfigWithoutFile(), *self.default_push_settings_args())
        settings1.device_id = "settings1_device_id"
        self.assertNotEqual(settings, settings1)





class TestMCSPushClient(SharedTestsUtilities):

    def setUp(self):
        self.push_client = MCSPushClient()
        self.args = ['certpath', [DIRECT_CONNECTION], ('user','pass')]

    def tearDown(self):
        self.push_client.stop_service()

    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_can_restart_twice(self, *mockargs):
        self.push_client._settings = MCSPushSetting.from_config(ConfigWithoutFile(), *self.args)
        self.push_client._start_service()
        received = self.send_and_receive_message('hello', self.push_client)
        self.assertEqual(received, 'hello')
        self.push_client.stop_service()
        self.push_client._start_service()
        received = self.send_and_receive_message('another', self.push_client)
        self.assertEqual(received, 'another')

    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_push_client_handle_config_changes(self, *mockargs):
        self.assertTrue(self.push_client.ensure_push_server_is_connected(ConfigWithoutFile(), *self.args))
        config = ConfigWithoutFile()
        config.set('pushServer1', 'new url')
        self.assertTrue(self.push_client.ensure_push_server_is_connected(config, *self.args))
        self.assertTrue(self.push_client.ensure_push_server_is_connected(config, *self.args))

    @mock.patch("logging.Logger.warning")
    def test_push_client_with_empty_proxy_list_fails_elegantly(self, *mockargs):
        self.push_client._settings = MCSPushSetting.from_config(ConfigWithoutFile(), 'certpath', [], ('user','pass'))
        self.assertRaises(MCSPushException, self.push_client._start_service)
        self.assertEqual(logging.Logger.warning.call_args_list[-1], mock.call("No connection methods available."))

    @mock.patch("logging.Logger.warning")
    @mock.patch("sseclient.SSEClient", side_effect=RuntimeError("failed to connect"))
    def test_push_client_with_no_good_connection_route_fails_elegantly(self, *mockargs):
        self.push_client._settings = MCSPushSetting.from_config(ConfigWithoutFile(), 'certpath', [DIRECT_CONNECTION, DIRECT_CONNECTION, DIRECT_CONNECTION], ('user', 'pass'))
        self.assertRaises(MCSPushException, self.push_client._start_service)
        self.assertEqual(logging.Logger.warning.call_args_list[-1], mock.call("Tried all connection methods and failed to connect to push server"))


    def test_push_client_returns_status_enum_when_applying_server_settings(self):
        # This is the case for when the connection to the Push Server cannot be established
        with mock.patch("sseclient.SSEClient", new_callable=SSEClientSimulateConnectionFailureFactory) as sseclient.SSEClient:
            self.assertFalse(self.push_client.ensure_push_server_is_connected(ConfigWithoutFile(), *self.args))

        with mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory) as sseclient.SSEClient:
            # this is the case for establishing connection to the push server
            self.assertTrue(self.push_client.ensure_push_server_is_connected(ConfigWithoutFile(), *self.args))
            # this is the case for a normal connected client without any settings change will just keep connected and nothing changes
            self.assertTrue(self.push_client.ensure_push_server_is_connected(ConfigWithoutFile(), *self.args))


if __name__ == '__main__':
    unittest.main()

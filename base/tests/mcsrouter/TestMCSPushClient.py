import unittest
import logging
from unittest import mock
import time
import selectors

import requests
import sseclient
import PathManager
import mcsrouter.utils.config
import mcsrouter.sophos_https as sophos_https
from types import SimpleNamespace
from mcsrouter.mcs_push_client import (MCSPushClientInternal, PipeChannel, MCSPushClient, MCSPushException,
                                       MCSPushSetting, PushClientStatus, MsgType)


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
        self.session = None
        self._msgs = []

    def push_message(self, msg):
        msg = SimpleNamespace(data=msg)
        self._msgs.append(msg)

    def __next__(self):
        if len(self._msgs) > 0:
            copy_msg = self._msgs.pop(0)
            self.resp.raw.clear()
            if len(self._msgs) > 0:
                self.resp.raw.notify()
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

def return_http_error_from_push_server(status_code=None, msg=None):
    response = requests.Response()
    response.status_code = status_code
    return requests.exceptions.HTTPError(msg, response=response)

class TestMCSPushClientInternal(SharedTestsUtilities):

    def setUp(self) -> None:
        self.client_internal = None

    def get_mcs_push_settings(self, proxies=[DIRECT_CONNECTION]):
        header = {
            "User-Agent": "Sophos MCS Client/0.1 x86_64 sessions regToke",
            "Authorization": f"Bearer <dummy_jwt_token>",
            "Content-Type": "application/json",
            "X-Device-ID": "thisisadeviceid",
            "X-Tenant-ID": "thisisatenantid",
            "Accept": "application/json",
        }
        sett = MCSPushSetting(url="url", cert="cert", expected_ping=10,
                              proxy_settings=proxies, connection_header=header,
                              device_id="thisisadeviceid", tenant_id="thisisatenantid")
        return sett

    def get_client(self, proxies=[DIRECT_CONNECTION], sett=None):
        if sett is None:
            sett = self.get_mcs_push_settings(proxies)
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

    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_can_receive_and_process_messages(self, *mockargs):
        client = self.get_client()
        client.start()
        received_msg = self.send_and_receive_message('hello', client)
        self.assertEqual(received_msg, 'hello')

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

    @mock.patch("logging.Logger.debug")
    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_multiple_quick_pings_are_handled(self, *mockargs):
        client = self.get_client()
        client.start()

        num_pings = 100
        global GlobalFakeSSeClient
        for _ in range(num_pings):
            GlobalFakeSSeClient.push_message('')
            GlobalFakeSSeClient.resp.raw.notify()

        time.sleep(0.3)

        for i in range(1, num_pings + 1):
            self.assertEqual(logging.Logger.debug.call_args_list[-i], mock.call("Server sent ping"))

        pending = client.pending_commands()
        self.assertEqual(len(pending), 0)

    @mock.patch("logging.Logger.debug")
    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_server_sends_ping_then_nothing_is_handled(self, *mockargs):
        settings = self.get_mcs_push_settings()
        settings.expected_ping = 0.1
        client = self.get_client(sett=settings)
        client.start()

        global GlobalFakeSSeClient
        GlobalFakeSSeClient.push_message('')
        GlobalFakeSSeClient.resp.raw.notify()

        time.sleep(0.1)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Server sent ping"))
        time.sleep(0.2)
        pending = client.pending_commands()
        for command in pending:
            self.assertEqual(command.msg_type, MsgType.Error)
            self.assertEqual(command.msg, "No Ping from Server")

    @mock.patch("logging.Logger.debug")
    @mock.patch("sseclient.SSEClient", side_effect=return_http_error_from_push_server(400, "test 400"))
    def test_multiple_different_http_400_errors_during_server_connection_attempt(self, *mockargs):
        settings = self.get_mcs_push_settings()
        settings.connection_header.pop("X-Device-ID")
        self.assertRaises(MCSPushException, lambda: self.get_client(sett=settings))
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Bad request, missing X-Device-ID in header"))

        settings = self.get_mcs_push_settings()
        settings.connection_header.pop("X-Tenant-ID")
        self.assertRaises(MCSPushException, lambda: self.get_client(sett=settings))
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Bad request, missing X-Tenant-ID in header"))

        settings = self.get_mcs_push_settings()
        settings.connection_header["X-Device-ID"] = "wrong device id"
        self.assertRaises(MCSPushException, lambda: self.get_client(sett=settings))
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Bad request, mismatch between Device ID in header and url"))

        settings = self.get_mcs_push_settings()
        settings.url = f"/v2/push/device/{settings.connection_header['X-Device-ID']}"
        self.assertRaises(MCSPushException, lambda: self.get_client(sett=settings))
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Bad request, reason unknown"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("sseclient.SSEClient", side_effect=return_http_error_from_push_server(401, "test 401"))
    def test_multiple_different_http_401_errors_during_server_connection_attempt(self, *mockargs):
        settings = self.get_mcs_push_settings()
        settings.connection_header.pop("Authorization")
        self.assertRaises(MCSPushException, lambda: self.get_client(sett=settings))
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Unauthorized, missing 'Authorization' in header"))

        settings = self.get_mcs_push_settings()
        settings.connection_header["Authorization"] = settings.connection_header["Authorization"][1:]
        self.assertRaises(MCSPushException, lambda: self.get_client(sett=settings))
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Unauthorized, 'Authorization' in header does not start with 'Bearer'"))

        settings = self.get_mcs_push_settings()
        self.assertRaises(MCSPushException, lambda: self.get_client(sett=settings))
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Unauthorized, unknown error with JWT token used in 'Authorization' header"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("sseclient.SSEClient", side_effect=return_http_error_from_push_server(403, "test 403"))
    def test_http_403_error_during_server_connection_attempt(self, *mockargs):
        settings = self.get_mcs_push_settings()
        self.assertRaises(MCSPushException, lambda: self.get_client(sett=settings))
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Forbidden, valid JWT but missing the required feature code"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("sseclient.SSEClient", side_effect=return_http_error_from_push_server(404, "test 404"))
    def test_http_404_error_during_server_connection_attempt(self, *mockargs):
        settings = self.get_mcs_push_settings()
        self.assertRaises(MCSPushException, lambda: self.get_client(sett=settings))
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Not found, URL must be for the format: /v2/push/device/<device_id>"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("sseclient.SSEClient", side_effect=return_http_error_from_push_server(405, "test 405"))
    def test_http_405_error_during_server_connection_attempt(self, *mockargs):
        settings = self.get_mcs_push_settings()
        self.assertRaises(MCSPushException, lambda: self.get_client(sett=settings))
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Method not allowed, invalid HTTP method used against URL"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("sseclient.SSEClient", side_effect=return_http_error_from_push_server(500, "test 500"))
    def test_http_500_error_during_server_connection_attempt(self, *mockargs):
        settings = self.get_mcs_push_settings()
        self.assertRaises(MCSPushException, lambda: self.get_client(sett=settings))
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Internal Server Error"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("sseclient.SSEClient", side_effect=return_http_error_from_push_server(503, "test 503"))
    def test_http_503_error_during_server_connection_attempt(self, *mockargs):
        settings = self.get_mcs_push_settings()
        self.assertRaises(MCSPushException, lambda: self.get_client(sett=settings))
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Service Unavailable, no subscriber nodes may be available"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("sseclient.SSEClient", side_effect=return_http_error_from_push_server(504, "test 504"))
    def test_http_504_error_during_server_connection_attempt(self, *mockargs):
        settings = self.get_mcs_push_settings()
        self.assertRaises(MCSPushException, lambda: self.get_client(sett=settings))
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Gateway Timeout, push server may be dealing with a heavy load"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("selectors.DefaultSelector.select", side_effect=return_http_error_from_push_server(400, "test 400"))
    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_multiple_different_http_400_errors_after_succesful_server_connection(self, *mockargs):
        client = self.get_client()
        client.messages.session, _, _ = client.get_requests_session()
        client.messages.session.headers.pop("X-Device-ID")
        client.start()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Bad request, missing X-Device-ID in header"))
        client.stop()

        client = self.get_client()
        client.messages.session, _, _ = client.get_requests_session()
        client.messages.session.headers.pop("X-Tenant-ID")
        client.start()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Bad request, missing X-Tenant-ID in header"))
        client.stop()

        client = self.get_client()
        client.messages.session, _, _ = client.get_requests_session()
        client.messages.session.headers["X-Device-ID"] = "wrong device id"
        client.start()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Bad request, mismatch between Device ID in header and url"))
        client.stop()

        client = self.get_client()
        client.messages.session, _, _ = client.get_requests_session()
        client._url = f"/v2/push/device/{client.messages.session.headers['X-Device-ID']}"
        client.start()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Bad request, reason unknown"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("selectors.DefaultSelector.select", side_effect=return_http_error_from_push_server(401, "test 401"))
    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_multiple_different_http_401_errors_after_succesful_server_connection(self, *mockargs):
        client = self.get_client()
        client.messages.session, _, _ = client.get_requests_session()
        client.messages.session.headers.pop("Authorization")
        client.start()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Unauthorized, missing 'Authorization' in header"))
        client.stop()

        client = self.get_client()
        client.messages.session, _, _ = client.get_requests_session()
        client.messages.session.headers["Authorization"] = client.messages.session.headers["Authorization"][1:]
        client.start()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Unauthorized, 'Authorization' in header does not start with 'Bearer'"))
        client.stop()

        client = self.get_client()
        client.messages.session, _, _ = client.get_requests_session()
        client.start()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Unauthorized, unknown error with JWT token used in 'Authorization' header"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("selectors.DefaultSelector.select", side_effect=return_http_error_from_push_server(403, "test 403"))
    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_http_403_error_after_succesful_server_connection(self, *mockargs):
        client = self.get_client()
        client.messages.session, _, _ = client.get_requests_session()
        client.start()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Forbidden, valid JWT but missing the required feature code"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("selectors.DefaultSelector.select", side_effect=return_http_error_from_push_server(404, "test 404"))
    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_http_404_error_after_succesful_server_connection(self, *mockargs):
        client = self.get_client()
        client.messages.session, _, _ = client.get_requests_session()
        client.start()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Not found, URL must be for the format: /v2/push/device/<device_id>"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("selectors.DefaultSelector.select", side_effect=return_http_error_from_push_server(405, "test 405"))
    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_http_405_error_after_succesful_server_connection(self, *mockargs):
        client = self.get_client()
        client.messages.session, _, _ = client.get_requests_session()
        client.start()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Method not allowed, invalid HTTP method used against URL"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("selectors.DefaultSelector.select", side_effect=return_http_error_from_push_server(500, "test 500"))
    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_http_500_error_after_succesful_server_connection(self, *mockargs):
        client = self.get_client()
        client.messages.session, _, _ = client.get_requests_session()
        client.start()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Internal Server Error"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("selectors.DefaultSelector.select", side_effect=return_http_error_from_push_server(503, "test 503"))
    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_http_503_error_after_succesful_server_connection(self, *mockargs):
        client = self.get_client()
        client.messages.session, _, _ = client.get_requests_session()
        client.start()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Service Unavailable, no subscriber nodes may be available"))

    @mock.patch("logging.Logger.debug")
    @mock.patch("selectors.DefaultSelector.select", side_effect=return_http_error_from_push_server(504, "test 504"))
    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_http_504_error_after_succesful_server_connection(self, *mockargs):
        client = self.get_client()
        client.messages.session, _, _ = client.get_requests_session()
        client.start()
        time.sleep(0.3)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Gateway Timeout, push server may be dealing with a heavy load"))

    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_server_does_not_send_any_response_after_successful_connection(self, *mockargs):
        client = self.get_client()
        client._expected_ping_interval = 0
        client.start()
        time.sleep(0.1)
        pending_commands = client.pending_commands()
        self.assertEqual(pending_commands[0].msg_type, MsgType.Error)
        self.assertEqual(pending_commands[0].msg, "No Ping from Server")

class ConfigWithoutFile( mcsrouter.utils.config.Config):
    def load(self, filename):
        pass
    def __init__(self):
        mcsrouter.utils.config.Config.__init__(self)
        self.set('pushServer1', 'value')
        self.set('pushPingTimeout', '10')
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

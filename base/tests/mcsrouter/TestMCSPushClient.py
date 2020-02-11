import unittest
import logging
import mock
import time
import selectors
import PathManager
import mcsrouter.utils.config
from types import SimpleNamespace
from mcsrouter.mcs_push_client import MCSPushClientInternal, PipeChannel, MCSPushClient, MCSPushException, MCSPushSetting


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

    def get_client(self):
        self.client_internal = MCSPushClientInternal("url", "cert", 10)
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
        sel = selectors.DefaultSelector()
        sel.register(client.notify_activity_pipe(), selectors.EVENT_READ, 'anything')
        self.assertEqual(len(sel.select(timeout=0.3)), 0)
        pending = client.pending_commands()
        self.assertEqual(len(pending), 0)
        self.assertEqual(logging.Logger.debug.call_args_list[-1], mock.call("Server sent ping"))


class ConfigWithoutFile( mcsrouter.utils.config.Config):
    def load(self, filename):
        pass
    def __init__(self):
        mcsrouter.utils.config.Config.__init__(self)
        self.set('pushServer1', 'value')
        self.set('PUSH_SERVER_CONNECTION_TIMEOUT', '10')

class TestMCSPushConfigSettings(unittest.TestCase):
    def test_extract_values_from_config(self):
        settings = MCSPushSetting.from_config(ConfigWithoutFile(), 'certpath')
        self.assertEqual(settings.url, 'value')
        self.assertEqual(settings.expected_ping,  10)
        self.assertEqual(settings.cert, 'certpath')

    def test_settings_equality(self):
        settings = MCSPushSetting.from_config(ConfigWithoutFile(), 'certpath')
        settings1 = MCSPushSetting.from_config(ConfigWithoutFile(), 'certpath')
        self.assertEqual(settings, settings1)
        settings1.url = 'changed'
        self.assertNotEqual(settings, settings1)



class TestMCSPushClient(SharedTestsUtilities):

    def setUp(self):
        self.push_client = MCSPushClient()

    def tearDown(self):
        self.push_client.stop_service()

    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_can_restart_twice(self, *mockargs):
        self.push_client._settings = MCSPushSetting.from_config(ConfigWithoutFile(), 'certpath')
        self.push_client._start_service()
        received = self.send_and_receive_message('hello', self.push_client)
        self.assertEqual(received, 'hello')
        self.push_client.stop_service()
        self.push_client._start_service()
        received = self.send_and_receive_message('another', self.push_client)
        self.assertEqual(received, 'another')

    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_push_client_handle_config_changes(self, *mockargs):
        self.assertTrue(self.push_client.check_push_server_settings_changed_and_reapply(ConfigWithoutFile(), 'certpath'))
        config = ConfigWithoutFile()
        config.set('pushServer1', 'new url')
        self.assertTrue(self.push_client.check_push_server_settings_changed_and_reapply(config, 'certpath'))
        self.assertFalse(self.push_client.check_push_server_settings_changed_and_reapply(config, 'certpath'))


    @mock.patch("sseclient.SSEClient", new_callable=SSEClientSimulateConnectionFailureFactory)
    def test_push_client_should_throw_if_failed_to_connect_to_push_service(self, *mockargs):
        self.assertRaises(MCSPushException, self.push_client.check_push_server_settings_changed_and_reapply, ConfigWithoutFile(), 'certpath')







if __name__ == '__main__':
    unittest.main()

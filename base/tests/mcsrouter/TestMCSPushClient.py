import unittest
import logging
import mock
import time
import selectors
import PathManager
from types import SimpleNamespace
from mcsrouter.mcs_push_client import MCSPushClient, PipeChannel


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


class MyTestCase(unittest.TestCase):

    def setUp(self) -> None:
        self.client = MCSPushClient("url", "cert", 10)

    def tearDown(self) -> None:
        self.client.stop()

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
        client = self.client
        client.start()
        time.sleep(1)
        client.stop()

    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_can_receive_and_process_messages(self, *mockargs):
        client = self.client
        client.start()
        global GlobalFakeSSeClient
        GlobalFakeSSeClient.push_message('hello')
        GlobalFakeSSeClient.resp.raw.notify()
        sel = selectors.DefaultSelector()
        sel.register(client.notify_activity_pipe(), selectors.EVENT_READ, 'anything')
        self.assertEqual(len(sel.select(timeout=0.3)), 1)
        pending = client.pending_commands()
        self.assertEqual(len(pending), 1)
        self.assertEqual(pending[0].msg, 'hello')
        client.stop()

    @mock.patch("logging.Logger.debug")
    @mock.patch("sseclient.SSEClient", new_callable=FakeSSEClientFactory)
    def test_pings_are_logged(self, *mockargs):
        client = self.client
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




if __name__ == '__main__':
    unittest.main()

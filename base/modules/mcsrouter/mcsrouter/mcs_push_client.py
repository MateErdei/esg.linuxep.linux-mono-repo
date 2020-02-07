#!/usr/bin/env python3
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.
"""
MCS Push Client
"""

import logging
import threading
import os
import errno
import sseclient
import socket
import selectors
from enum import Enum
import mcsrouter.utils.signal_handler
logger = logging.getLogger(__name__)

class MsgType(Enum):
    MCSCommand = 1
    Error = 2


class PushClientCommand:
    def __init__(self, msg_type: MsgType, msg_content: str):
        self.msg_type = msg_type
        self.msg = msg_content

class PipeChannel:
    def __init__(self):
        self.read, self.write = mcsrouter.utils.signal_handler.create_pipe()

    def notify(self):
        os.write(self.write, b'.')

    def clear(self):
        while True:
            try:
                if os.read(
                        self.read,
                        1024) is None:
                    break
            except OSError as err:
                if err.errno == errno.EAGAIN or err.errno == errno.EWOULDBLOCK:
                    break
                else:
                    raise

    def __del__(self):
        os.close(self.read)
        os.close(self.write)

class MCSPushClient(threading.Thread):
    def __init__(self, url: str, cert: str, expected_ping_interval: int):
        threading.Thread.__init__(self)
        self._url = url
        self._cert = cert
        self._expected_ping_interval = expected_ping_interval
        self._notify_push_client_channel = PipeChannel()
        self._notify_mcsrouter_channel = PipeChannel()
        self._pending_commands = []

    def __del__(self):
        self.stop()
        self.join()

    def run(self):
        try:
            messages = sseclient.SSEClient(self._url, verify=self._cert)
            sel = selectors.DefaultSelector()
            sel.register(messages.resp.raw, selectors.EVENT_READ, 'push')
            sel.register(self._notify_push_client_channel.read, selectors.EVENT_READ, 'stop')
            while True:
                events = sel.select(self._expected_ping_interval)
                if not events:
                    self._push_error_message('No Ping from Server')
                for key, mask in events:
                    if key.data == 'push':
                        msg_event = next(messages)
                        msg = msg_event.data
                        if msg:
                            self._push_message(msg)
                        else:
                            logger.debug("Server sent ping")
                    elif key.data == 'stop':
                        logger.info("MCS Push Client main loop finished")
                        return
        except Exception as ex:
            self._push_error_message('Push client Failure : {}'.format(ex))
            raise

    def stop(self):
        self._notify_push_client_channel.notify()

    def notify_activity_pipe(self):
        """Clients should monitor the file descriptor returned by this method to know when there are 'new messages'
        from the push system
        """
        return self._notify_mcsrouter_channel.read

    def pending_commands(self):
        copy_pending = self._pending_commands[:]
        self._pending_commands = []
        self._notify_mcsrouter_channel.clear()
        return copy_pending

    def _push_message(self, msg:str):
        self._pending_commands.append( PushClientCommand(MsgType.MCSCommand, msg))
        self._notify_mcsrouter_channel.notify()

    def _push_error_message(self, msg:str):
        self._pending_commands.append( PushClientCommand(MsgType.Error, msg))
        self._notify_mcsrouter_channel.notify()


if __name__ == "__main__":
    url = 'https://localhost:8459/mcs/push/endpoint/me'
    cert = '/home/pair/gitrepos/sspl-tools/everest-base/testUtils/SupportFiles/CloudAutomation/root-ca.crt.pem'

    import sys

    logger.setLevel(logging.DEBUG)
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(logging.DEBUG)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    handler.setFormatter(formatter)
    logger.addHandler(handler)

    client = MCSPushClient(url,cert,12)
    client.start()
    import time
    time.sleep(30)
    for command in client.pending_commands():
        print(command.msg)
    client.stop()

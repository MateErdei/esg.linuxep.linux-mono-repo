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
import requests
import sseclient
import selectors
from enum import Enum
from urllib.parse import urlparse
import mcsrouter.utils.signal_handler
import mcsrouter.sophos_https
from . import sophos_https

LOGGER = logging.getLogger(__name__)

class MsgType(Enum):
    MCSCommand = 1
    Error = 2
    Wakeup = 3

class PushClientStatus(Enum):
    NothingChanged = 1
    Connected = 2
    Error = 3

class PushClientCommand:
    def __init__(self, msg_type: MsgType, msg_content: str):
        self.msg_type = msg_type
        self.msg = msg_content

    def __repr__(self):
        if self.msg_type == MsgType.Error:
            return "Error: {}".format(self.msg)
        return "Command: {}".format(self.msg)

class PipeChannel:
    """
    Wraps a read and write pipe which can be used for communication between threads
    """
    def __init__(self):
        self.read, self.write = mcsrouter.utils.signal_handler.create_pipe()

    def fileno(self):
        """To be used for selectors"""
        return self.read

    def notify(self):
        """
        send an arbitrary message through the pipe to notify the listener
        :return:
        """
        os.write(self.write, b'.')

    def clear(self):
        while True:
            try:
                if os.read(self.read, 1024) is None:
                    break
            except OSError as err:
                if err.errno == errno.EAGAIN or err.errno == errno.EWOULDBLOCK:
                    break
                raise

    def __del__(self):
        os.close(self.read)
        os.close(self.write)

class MCSPushException(RuntimeError):
    pass

class MCSPushSetting:
    @staticmethod
    def from_config(config, cert, proxy_settings):
        push_server_url = config.get_default("pushServer1", None)
        mcs_id = config.get_default("MCSID", None)
        if push_server_url and mcs_id:
            url = os.path.join(push_server_url, "push/endpoint/", mcs_id)
        else:
            url = None
        expected_ping = config.get_int("PUSH_SERVER_CONNECTION_TIMEOUT")
        certs = cert
        return MCSPushSetting(url, certs, expected_ping, proxy_settings)

    def as_tuple(self):
        return self.url, self.cert, self.expected_ping, self.proxy_settings

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.as_tuple() == other.as_tuple()
        return False

    def __init__(self, url=None, cert=None, expected_ping=None, proxy_settings=None):
        self.url = url
        self.cert = cert
        self.expected_ping = expected_ping
        self.proxy_settings = proxy_settings

class MCSPushClient:
    def __init__(self):
        self._notify_mcsrouter_channel = PipeChannel()
        self._push_client_impl = None
        self._settings = MCSPushSetting()

    def _start_service(self):
        """Raise exception if cannot establish connection with the push server"""

        self.stop_service()
        try:
            self._notify_mcsrouter_channel.clear()
            url, cert, expected_ping, proxy_settings = self._settings.as_tuple()
            LOGGER.debug("Settings for push client: url: {}, ping {} and cert {}".format(url, expected_ping, cert))
            if not url:
                return
            self._push_client_impl = MCSPushClientInternal(url, cert, expected_ping, proxy_settings, self._notify_mcsrouter_channel)
            self._push_client_impl.start()
            LOGGER.info("Established MCS Push Connection")
        except Exception as e:
            LOGGER.warning(str(e))
            raise MCSPushException("Failed to start MCS Push Client service")

    def stop_service(self):
        if not self._push_client_impl:
            return
        self._push_client_impl.stop()
        self._push_client_impl = None
        LOGGER.info("MCS push client stopped")

    def pending_commands(self):
        if not self._push_client_impl:
            return []
        return self._push_client_impl.pending_commands()

    def notify_activity_pipe(self):
        return self._notify_mcsrouter_channel

    def is_service_active(self):
        if not self._push_client_impl:
            return False
        return self._push_client_impl.is_alive()

    def ensure_push_server_is_connected(self, config, cert, proxy_settings):
        # retrieve push server url and compare with the current one
        settings = MCSPushSetting.from_config(config, cert, proxy_settings)
        need_start = False
        try:
            if settings != self._settings:
                need_start = True
                LOGGER.info("Push Server settings changed. Applying it")
            elif not self.is_service_active() and self._settings.url:
                LOGGER.info("Trying to re-connect to Push Server")
                need_start = True

            if need_start:
                self._settings = settings
                self._start_service()
        except Exception as ex:
            LOGGER.warning(str(ex))

        return self.is_service_active()


class MCSPushClientInternal(threading.Thread):
    def __init__(self, url: str, cert: str, expected_ping_interval: int, proxy_settings, mcsrouter_channel=PipeChannel()):
        threading.Thread.__init__(self)
        self._url = url
        self._cert = cert
        self._expected_ping_interval = expected_ping_interval
        self._proxy_settings = proxy_settings
        self._notify_push_client_channel = PipeChannel()
        self._notify_mcsrouter_channel = mcsrouter_channel
        self._pending_commands = []
        self._pending_commands_lock = threading.Lock()
        self.messages = self._create_sse_client()

    def _create_sse_client(self):
        for proxy in self._proxy_settings:
            self._proxy = proxy
            LOGGER.info(self.__attempting_connection_message())
            session = self.get_requests_session()
            try:
                messages = sseclient.SSEClient(self._url, session=session)
                LOGGER.info(self.__successful_connection_message())
                return messages
            except Exception as exception:
                LOGGER.warning("{}: {}".format(self.__failed_connection_message(), str(exception)))
        else:
            raise MCSPushException("Failed to connect to {}".format(self.__log_url()))

    def __log_url(self):
        return urlparse(self._url).netloc

    def __attempting_connection_message(self):
        if self._proxy.is_configured():
            return "Trying push connection to {} via {}".format(self.__log_url(), self._proxy.log_address())
        else:
            return "Trying push connection directly to {}".format(self.__log_url())


    def __successful_connection_message(self):
        if self._proxy.is_configured():
            return "Push client successfully connected to {} via {}".format(self.__log_url(), self._proxy.log_address())
        else:
            return "Push client successfully connected to {} directly".format(self.__log_url())

    def __failed_connection_message(self):
        if self._proxy.is_configured():
            return "Failed to connect to {} via {}".format(self.__log_url(), self._proxy.log_address())
        else:
            return "Failed to connect to {} directly".format(self.__log_url())

    def get_requests_session(self):
        session = requests.Session()
        session.verify = self._cert
        if self._proxy.is_configured():
            session.proxies = {
                'http': self._proxy.address(),
                'https': self._proxy.address()
            }
        return session

    def run(self):
        try:

            sel = selectors.DefaultSelector()
            sel.register(self.messages.resp.raw, selectors.EVENT_READ, 'push')
            sel.register(self._notify_push_client_channel, selectors.EVENT_READ, 'stop')
            while True:
                # This call will block for the duration of the passed argument
                events = sel.select(self._expected_ping_interval)
                if not events:
                    self._append_command(MsgType.Error, 'No Ping from Server')
                for key, mask in events:
                    if key.data == 'push':
                        msg_event = next(self.messages)
                        if msg_event.data:
                            msg_type = MsgType.MCSCommand
                            if is_msg_wakeup(msg_event.data):
                                msg_type = MsgType.Wakeup
                            # we don't want to log the full message, just the beginning is enough 
                            LOGGER.info("Received command: {}".format(msg_event.data[:100]))
                            self._append_command(msg_type, msg_event.data)
                        else:
                            LOGGER.debug("Server sent ping")
                    elif key.data == 'stop':
                        LOGGER.info("MCS Push Client main loop finished")
                        return
        except Exception as ex:
            self._append_command(MsgType.Error, 'Push client Failure : {}'.format(ex))

    def stop(self):
        self._notify_push_client_channel.notify()
        try:
            self.join()
        except RuntimeError:
            # either because it has already been joined or because it has not been started. Not to worry
            pass

    def notify_activity_pipe(self):
        """Clients should monitor the file descriptor returned by this method to know when there are 'new messages'
        from the push system
        """
        return self._notify_mcsrouter_channel.read

    def pending_commands(self):
        try:
            self._pending_commands_lock.acquire()
            copy_pending = self._pending_commands[:]
            self._pending_commands = []
        finally:
            self._pending_commands_lock.release()
        self._notify_mcsrouter_channel.clear()
        return copy_pending

    def _append_command(self, msg_type, msg):
        try:
            self._pending_commands_lock.acquire()
            self._pending_commands.append(PushClientCommand(msg_type, msg))
        finally:
            self._pending_commands_lock.release()
        self._notify_mcsrouter_channel.notify()

def is_msg_wakeup(msg):
    return "sophos.mgt.action.GetCommands" in msg

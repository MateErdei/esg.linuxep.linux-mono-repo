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
import requests
import urllib3
import mcsrouter.digestproxyworkaround as digestproxyworkaround
urllib3.connectionpool.HTTPSConnectionPool.ConnectionCls = digestproxyworkaround.ReplaceConnection
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
    def from_config(config, cert, proxy_settings, connection_header):
        """

        :param config: mcs config
        :param cert: certs to use for connection
        :param proxy_settings: list of sophos_https.Proxy objects. It is ordered by connection priority and
        :param connection_header: header to use when connecting with V2 Push Server
        includes a direct connection if applicable
        :return: MCSPushSetting object
        """
        push_server_url = config.get_default("pushServer1", None)

        device_id = config.get_default("device_id", None)
        if push_server_url and device_id:
            url = os.path.join(push_server_url, "v2/push/device", device_id)
        else:
            url = None
        expected_ping = config.get_int("pushPingTimeout")
        certs = cert
        tenant_id = config.get_default("tenant_id", None)
        return MCSPushSetting(url, certs, expected_ping, proxy_settings, connection_header, device_id, tenant_id)

    def as_tuple(self):
        return self.url, self.cert, self.expected_ping, self.proxy_settings, self.device_id, self.tenant_id, self.connection_header

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.as_tuple() == other.as_tuple()
        return False

    def __str__(self):
        return "url {}, ping {} and cert {}".format(self.url, self.expected_ping, self.cert)

    def __init__(self, url=None, cert=None, expected_ping=None, proxy_settings=None, connection_header=None, device_id=None, tenant_id=None):
        self.url = url
        self.cert = cert
        self.expected_ping = expected_ping
        self.proxy_settings = proxy_settings
        self.device_id = device_id
        self.tenant_id = tenant_id
        self.connection_header = connection_header


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
            LOGGER.debug("Settings for push client: {}".format(self._settings))
            if not self._settings.url:
                LOGGER.info("Push client not connecting, no url given")
                return
            self._push_client_impl = MCSPushClientInternal(self._settings,
                                                           self._notify_mcsrouter_channel)
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

    def ensure_push_server_is_connected(self, config, cert, proxy_settings, connection_header):
        # retrieve push server url and compare with the current one
        settings = MCSPushSetting.from_config(config, cert, proxy_settings, connection_header)
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
            exception_string = str(ex)
            LOGGER.warning(exception_string)

        return self.is_service_active()


class MCSPushClientInternal(threading.Thread):
    def __init__(self, settings: MCSPushSetting,
                 mcsrouter_channel=PipeChannel()):
        threading.Thread.__init__(self)
        self._url = settings.url
        self._cert = settings.cert
        self._expected_ping_interval = settings.expected_ping
        self._proxy_settings = settings.proxy_settings
        self.__device_id = settings.device_id
        self.__tenant_id = settings.tenant_id
        self.connection_header = settings.connection_header
        self._notify_push_client_channel = PipeChannel()
        self._notify_mcsrouter_channel = mcsrouter_channel
        self._pending_commands = []
        self._pending_commands_lock = threading.Lock()
        self.messages = self._create_sse_client()

    def _log_http_error_reason(self, response, session):
        status_code = response.status_code
        if status_code == 400:
            if "X-Device-ID" not in session.headers:
                LOGGER.debug("Bad request, missing X-Device-ID in header")
            elif "X-Tenant-ID" not in session.headers:
                LOGGER.debug("Bad request, missing X-Tenant-ID in header")
            elif session.headers["X-Device-ID"] != self._url.split('/')[-1]:
                LOGGER.debug("Bad request, mismatch between Device ID in header and url")
            else:
                LOGGER.debug("Bad request, reason unknown")
        elif status_code == 401:
            if "Authorization" not in session.headers:
                LOGGER.debug("Unauthorized, missing 'Authorization' in header")
            elif not session.headers["Authorization"].startswith("Bearer"):
                LOGGER.debug("Unauthorized, 'Authorization' in header does not start with 'Bearer'")
            else:
                LOGGER.debug("Unauthorized, unknown error with JWT token used in 'Authorization' header")
        elif status_code == 403:
            LOGGER.debug("Forbidden, valid JWT but missing the required feature code")
        elif status_code == 404:
            LOGGER.debug("Not found, URL must be for the format: /v2/push/device/<device_id>")
        elif status_code == 405:
            LOGGER.debug("Method not allowed, invalid HTTP method used against URL")
        elif status_code == 500:
            LOGGER.debug("Internal Server Error")
        elif status_code == 503:
            LOGGER.debug("Service Unavailable, no subscriber nodes may be available")
        elif status_code == 504:
            LOGGER.debug("Gateway Timeout, push server may be dealing with a heavy load")

    def _create_sse_client(self):
        if len(self._proxy_settings) == 0:
            raise MCSPushException("No connection methods available." .format(self.__log_url()))

        for proxy in self._proxy_settings:
            self._proxy = proxy
            LOGGER.info(self.__attempting_connection_message())
            session, proxy_username, proxy_password = self.get_requests_session()
            LOGGER.debug("Try connection to push server via this route: {}".format(session.proxies))
            LOGGER.debug(f"Session headers used: {session.headers}")
            LOGGER.debug(f"URL used: {self._url}")
            try:
                digestproxyworkaround.GLOBALAUTHENTICATION.set(proxy_username, proxy_password)
                messages = sseclient.SSEClient(self._url, session=session, timeout=30)
                LOGGER.info(self.__successful_connection_message())
                LOGGER.debug("success in connecting to push server")
                return messages
            except requests.exceptions.HTTPError as exception:
                self._log_http_error_reason(exception.response, session)
                LOGGER.warning("{}: {}".format(self.__failed_connection_message(), str(exception)))
            except Exception as exception:
                LOGGER.warning("{}: {}".format(self.__failed_connection_message(), str(exception)))
            finally:
                digestproxyworkaround.GLOBALAUTHENTICATION.clear()
        else:
            raise MCSPushException(f"Tried all connection methods and failed to connect to {self.__log_url()}")

    def __log_url(self):
        loggable_url = urlparse(self._url).netloc
        if loggable_url == "":
            return "push server"
        return loggable_url

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
                'http': self._proxy.address(with_full_uri=False),
                'https': self._proxy.address(with_full_uri=False)
            }
        session.trust_env = False
        session.headers = self.connection_header
        return session, self._proxy.m_username, self._proxy.m_password

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
                        self.messages.resp.close()
                        LOGGER.info("MCS Push Client main loop finished")
                        return
        except requests.exceptions.HTTPError as exception:
            self._log_http_error_reason(exception.response, self.messages.session)
            self._append_command(MsgType.Error, f'Push client Failure : {exception}')
        except StopIteration as stop_iteration_exception:
            self._append_command(MsgType.Error, 'Push client lost connection to server : sseclient raised StopIteration exception')
        except Exception as ex:
            self._append_command(MsgType.Error, f'Push client Failure : {ex}')

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

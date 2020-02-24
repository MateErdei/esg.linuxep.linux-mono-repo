#!/usr/bin/env python3

import requests
import sys
from sseclient import SSEClient
import subprocess
import time
import os
import PathManager
try:
    from robot.api import logger
except:
    import logging
    logger = logging.getLogger(__name__)

class RequestsSessionAllowRedirectWithAuth(requests.Session):
    def __init__(self):
        """
        see mcs_push_client.py:
        """
        requests.Session.__init__(self)

    def should_strip_auth(self, old_url, new_url):
        return False


class PushServerUtils:
    """Utilities to verify the MCS Push Service Requirements that can be used with the Robot Framework.
       For examples, see TestMCSPushServer
    """
    def __init__(self):
        self._client = None
        self._server = None
        self._port = 8459
        self._cert = os.path.join( PathManager.get_support_file_path(), 'CloudAutomation/root-ca.crt.pem')
        self.tmp_path = os.path.join(".", "tmp")
        self.cloud_server_log = os.path.join(self.tmp_path, "push_server_log.log")
        self.push_url_pattern = 'https://localhost:{}/mcs/push/endpoint/thisendpoint'
        if os.path.exists(self.cloud_server_log):
            os.remove(self.cloud_server_log)

    def __del__(self):
        self.shutdown_mcs_push_server()

    def set_port(self, port):
        self._port = port

    def set_cert(self, cert):
        self._cert = cert

    def send_message_to_push_server(self, message):
        """Use the subscription channel to send messages to all the clients connected to the push server."""
        r = requests.post('https://localhost:{}/mcs/push/sendmessage'.format(self._port), data=message, verify=self._cert)
        if r.status_code != 200:
            raise AssertionError("Send message Failed with code {} and Text {}".format(r.status_code, r.text))

    def send_message_to_push_server_from_file(self, file):
        """Use the subscription channel to send the contents of a file
         to all the clients connected to the push server."""
        if os.path.isfile(file):
            with open(file, "r") as file_handle:
                message = file_handle.read()
                self.send_message_to_push_server(message)

    def configure_push_server_to_require_auth(self, authorization):
        """Configure the Push Server to validate the Authorization Header"""
        r = requests.put('https://localhost:{}/mcs/push/authorization'.format(self._port), data=authorization, verify=self._cert)
        if r.status_code != 200:
            raise AssertionError("Send message Failed with code {} and Text {}".format(r.status_code, r.text))

    def configure_push_server_to_ping_interval(self, ping_time):
        """Configure the Push Server Keep Alive Ping Interval.
           The parameter must be integer or a string that can be set to  v = int(ping_time)
        """
        r = requests.put('https://localhost:{}/mcs/push/ping_interval'.format(self._port), data=str(ping_time), verify=self._cert)
        if r.status_code != 200:
            raise AssertionError("Send message Failed with code {} and Text {}".format(r.status_code, r.text))

    def instruct_push_server_to_close_connections(self):
        r = requests.post('https://localhost:{}/mcs/push/sendmessage'.format(self._port), data='stop', verify=self._cert)
        if r.status_code != 200:
            raise AssertionError("Send message to close connections failed with code {} and Text {}".format(r.status_code, r.text))

    def start_sse_client(self, timeout=100, authorization="DefaultAuthorization", via_cloud_server=False):
        """
        Start the Server Sent Event Client that will connect to the push server and receive messages from it.

        :param timeout: Defines the expected ping_time interval and it will detect server non-responsive (timeout) if the server does not keep sending a keep alive message in that interval.
        :param authorization:  Defines the Authorization header to be used if not empty string. Otherwise no Authorization Header is sent.
        :param via_cloud_server: Define the route to connect to the server. If via_cloud_server is true, it will connect to the mcs fake server which will redirect to the push server.

        After start_sse_client is correctly executed, new messages can be received with the command: next_sse_message

        """
        sse_args = {'verify': self._cert, 'retry': 0,
                    'timeout': timeout,
                    'headers': {'Authorization': authorization}
                    }

        if via_cloud_server:
            # it has been found that in order to work the redirect must deal with striping authorization.
            url = self.push_url_pattern.format(4443)
            session = RequestsSessionAllowRedirectWithAuth()
            session.headers['Authorization'] = authorization
            session.verify = self._cert
            copy_args = {'session': session, 'retry': 0, 'timeout': timeout}
            sse_args = copy_args
        else:
            url = self.push_url_pattern.format(self._port)

        self._client = SSEClient(url, **sse_args)

    def next_sse_message(self):
        """
        Read the next Server Sent Message.
        Special Handling is done for some expected situations.
        :return: 'stop' if StopIteration is raised.
                 'abort' if connection is lost
                 'ping' if the keep alive ping message is detected
                 message: the content of the message sent by the server.
        """
        try:
            data = next(self._client)
        except StopIteration:
            return 'stop'
        except requests.exceptions.ConnectionError as ex:
            logger.info("Exception: type {} content {}".format(type(ex), ex))
            return 'abort'
        text = data.data
        if not text:
            return "ping"
        return text

    def verify_cloud_server_redirect(self):
        """
        Verify that the cloudServer.py sends the redirect response on the request to connect to the push service.
        :return:
        """
        url_pattern = self.push_url_pattern
        r = requests.get(url_pattern.format(4443), allow_redirects=False, verify=self._cert)
        if r.status_code != 307:
            raise AssertionError("VerifyCloudServerRedirect failed code {} and Text {}".format(r.status_code, r.text))
        print(dict(r.headers))
        redirect_url = r.headers['Location']
        if redirect_url != url_pattern.format(self._port):
            raise AssertionError("Location differs from the expected. Given Location {}".format(redirect_url))

    def start_mcs_push_server(self, authorization=None):
        """Utility function to simplify starting the mcs push server. It executes MockMCSPushServer.py """
        server_path = os.path.join(os.path.dirname(__file__), 'MockMCSPushServer.py')
        args= [ sys.executable,
                server_path,
                '--logpath',
                self.cloud_server_log
        ]
        self._server = subprocess.Popen(args)
        time.sleep(1)
        if authorization:
            self.configure_push_server_to_require_auth(authorization=authorization)

    def mcs_push_server_log(self):
        """
        :return: The content of the logs generated by the MCS Push Server
        """
        with open(self.cloud_server_log, 'r') as file_handler:
            return file_handler.read()

    def client_close(self):
        """Instruct the sse client to close connection"""
        if self._client:
            self._client.resp.close()
            self._client = None

    def server_close(self):
        """Stops the MCS Push Server (MockMCSPushServer)"""
        if self._server:
            self._server.kill()
            out, err = self._server.communicate()
            if out:
                logger.info("Server Info: {}".format(out.decode()))
            if err:
                logger.info("Server Stderr: {}".format(err.decode()))
            self._server = None

    def check_server_stops_connection(self):
        """Verify that the sse client detects server instruction to stop the streaming (close connection)."""
        # the following code does not work with the sseclient,
        # although the close method does work with the browser.
        # Check keyword: Client Should Detect Push Server Disconnection
        self.instruct_push_server_to_close_connections()
        messages = [self.next_sse_message()] * 3
        if 'stop' not in messages:
            raise AssertionError("Expected stop but received msg: {}".format(messages))

    def check_ping_time_interval(self, expected_interval=3):
        """Verify that the MCS Push Server obeys the configuration of the ping_time
        sending keep alive messages around the configured time
        see: configure_push_server_to_ping_interval
        """
        msg1 = self.next_sse_message()
        start = time.time()
        msg2 = self.next_sse_message()
        stop = time.time()
        if not (msg1 == msg2 and msg1 == 'ping'):
            raise AssertionError("Expected 2 pings, received {} and {}".format(msg1, msg2))
        elapsed = stop - start
        if not expected_interval * 0.3 < elapsed < expected_interval*1.3:
            raise AssertionError("Expected ping interval to be close to the timeout {}, but is: {} ".format(expected_interval, elapsed))

    def check_mcs_push_message_sent(self, message):
        """Verify that messages sent to sse client via the MCS Push Server is received correctly."""
        self.send_message_to_push_server(message)
        unescaped_client_received_message = self.next_sse_message()
        client_received_message = unescaped_client_received_message.replace('\\n', '\n')
        if client_received_message != message:
            raise AssertionError("Message differ. Expected {}.\nReceived {}".format(message, client_received_message))

    def check_client_not_authorized(self, authorization):
        """Verify that if MCS Push Server is configured to check Authorization header it will reject connections
        that does not present the valid authorization"""
        try:
            self.start_sse_client(authorization=authorization)
            raise AssertionError("Client connected with the authorization set to {}".format(authorization))
        except requests.exceptions.HTTPError as ex:
            if ex.response.status_code == 401:
                return
            raise AssertionError("Client failed for a different reason. Ex={}".format(ex))

    def shutdown_mcs_push_server(self):
        """Shutdown the Push Server and the Client cleanly to be used in TearDown to avoid"""
        self.client_close()
        self.server_close()


# you can run the push server tests by:
# python3 libs/PushServerUtils.py
# it is easier to verify that things are working without needing the full test suite running
if __name__ == "__main__":
    string_to_send = "hello from send"
    if len(sys.argv) == 2:
        string_to_send = sys.argv[1]
    p = PushServerUtils()
    p.start_mcs_push_server()
    p.start_sse_client()
    p.check_mcs_push_message_sent("another")

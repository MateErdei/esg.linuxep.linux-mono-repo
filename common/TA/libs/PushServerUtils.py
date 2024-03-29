#!/usr/bin/env python3
import ssl
from http.client import HTTPSConnection
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


class PushServerUtils:
    """Utilities to verify the MCS Push Service Requirements that can be used with the Robot Framework.
       For examples, see TestMCSPushServer
    """
    def __init__(self):
        self._client = None
        self._server = None
        self._port = 8459
        self._cert = os.path.join(PathManager.get_utils_path(), 'server_certs/server-root.crt')
        self.tmp_path = os.path.join(".", "tmp")
        self.cloud_server_log = os.path.join(self.tmp_path, "push_server.log")
        self.push_url_pattern = 'https://localhost:{}/mcs/v2/push/device/thisisadevice'
        if os.path.exists(self.cloud_server_log):
            os.remove(self.cloud_server_log)

    def __del__(self):
        self.shutdown_mcs_push_server()

    def set_port(self, port):
        self._port = port

    def set_cert(self, cert):
        self._cert = cert

    def send_message_to_push_server(self, message=None):
        context = ssl.create_default_context(cafile=self._cert)
        conn = HTTPSConnection(host="localhost", port=self._port, context=context)
        conn._http_vsn_str = 'HTTP/1.1'
        conn._http_vsn = 11
        conn.request('POST', '/mcs/push/sendmessage', body=message)
        r = conn.getresponse()
        if r.status != 200:
            raise AssertionError("Send message Failed with code {} and Text {}".format(r.status_code, r.text))

    def send_message_to_push_server_from_file(self, file):
        """Use the subscription channel to send the contents of a file
         to all the clients connected to the push server."""
        if os.path.isfile(file):
            with open(file, "r") as file_handle:
                message = file_handle.read()
                self.send_message_to_push_server(message)

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

    def start_sse_client(self, timeout=100, via_cloud_server=False):
        """
        Start the Server Sent Event Client that will connect to the push server and receive messages from it.

        :param timeout: Defines the expected ping_time interval and it will detect server non-responsive (timeout) if the server does not keep sending a keep alive message in that interval.
        :param via_cloud_server: Define the route to connect to the server. If via_cloud_server is true, it will connect to the mcs fake server which will redirect to the push server.

        After start_sse_client is correctly executed, new messages can be received with the command: next_sse_message

        """
        sse_args = {'verify': self._cert, 'retry': 0,
                    'timeout': timeout
                    }

        if via_cloud_server:
            url = self.push_url_pattern.format(4443)
            session = requests.Session()
            session.headers["Authorization"] = "Bearer <JWT_TOKEN>"
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
        except (requests.exceptions.ConnectionError, ssl.SSLError) as ex:
            logger.info("Exception: type {} content {}".format(type(ex), ex))

            # For cases where python < 3.9 and OpenSSL3 are being used, the following error will be thrown
            # Newer python versions have fixes to work around this error, but they were not backported to python 3.7/3.8
            # https://github.com/dask/distributed/issues/5607
            if isinstance(ex, ssl.SSLError):
                if "[SSL: KRB5_S_TKT_NYV] unexpected eof while reading" not in str(ex):
                    logger.error("Unexpected ssl.SSLError")
                    raise ex

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
        r = requests.get(url_pattern.format(4443), allow_redirects=False, verify=self._cert, headers={"Authorization":"Bearer allow me"})
        if r.status_code != 307:
            raise AssertionError("VerifyCloudServerRedirect failed code {} and Text {}".format(r.status_code, r.text))
        print(dict(r.headers))
        redirect_url = r.headers['Location']
        if redirect_url != url_pattern.format(self._port):
            raise AssertionError("Location differs from the expected. Given Location {}".format(redirect_url))

    def start_mcs_push_server(self):
        """Utility function to simplify starting the mcs push server. It executes MockMCSPushServer.py """
        if self._server:
            logger.error("Attempting to start server while it is already running!")
        server_path = os.path.join(os.path.dirname(__file__), 'MockMCSPushServer.py')
        args = [
            sys.executable,
            server_path,
            '--logpath',
            self.cloud_server_log,
        ]
        self._server = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        start = time.time()
        while time.time() < start + 1:
            if self._server.poll():
                output = self._server.communicate()[0]
                code = self._server.returncode
                raise AssertionError(f"MockMCSPushServer.py immediately exited: {code}: {output}")
            if os.path.isfile(self.cloud_server_log):
                logger.info("MockMCSPushServer.py log created")
                if "Listening on port:" in self.mcs_push_server_log():
                    logger.info("MockMCSPushServer.py listening")
                    break
            time.sleep(0.1)

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
                logger.info(f"MockMCSPushServer output: {out.decode()}")
            if err:
                logger.info(f"Server Stderr: {err.decode()}")
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

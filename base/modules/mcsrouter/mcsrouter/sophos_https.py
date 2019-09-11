#!/usr/bin/env python
"""
sophos_https Module
"""

import os
import urllib.request, urllib.error, urllib.parse
import socket
import ssl
import http.client
import base64

# urllib.parse in python 3
import urllib.parse

# pylint: disable=relative-import
from . import proxy_authorization
from .mcsclient import mcs_exception
LOGGER = None


def info(*message):
    """
    info
    """
    if LOGGER is None:
        return
    LOGGER.info(*message)


def split_host_port(host_port, default_port=80):
    """
    Split a hostname:port into (hostname,port) with a default port
    """
    host_name_and_port = host_port.rsplit(":", 1)
    host = host_name_and_port[0]
    if len(host_name_and_port) == 2:
        if host_name_and_port[1].endswith("/"):
            host_name_and_port[1] = host_name_and_port[1][:-1]
        try:
            port = int(host_name_and_port[1])
        except ValueError:
            port = default_port
    else:
        port = default_port
    return (host, port)


def get_proxy(proxy=None):
    """
    get_proxy
    """
    if not proxy:
        proxy = os.environ.get("https_proxy", None)
    if proxy == "noproxy:":
        proxy_host = None
        proxy_port = None
    elif proxy is not None:
        if proxy.startswith("http://"):
            proxy = proxy[len("http://"):]
        if proxy.endswith("/"):
            proxy = proxy[:-1]
        (proxy_host, proxy_port) = split_host_port(proxy, 8080)
    else:
        proxy_host = None
        proxy_port = None

    return (proxy_host, proxy_port)


class Proxy(object):
    """
    Proxy
    """

    def __init__(
            self,
            host=None,
            port=None,
            username=None,
            password=None,
            relay_id=None):
        """
        __init__
        """
        # pylint: disable=too-many-arguments
        if host in (None, "noproxy:", ""):
            # If we don't have a host then we don't have a proxy at all
            host = None
            port = None
            username = None
            password = None
            relay_id = None
        else:
            if host.startswith("http://"):
                host = host[len("http://"):]

            if host.startswith("https://"):
                host = host[len("https://"):]

            if "@" in host:
                credentials, host_port = host.rsplit("@", 1)
                credentials = credentials.split(":", 1)

                if username is None:
                    username = credentials[0]
                if password is None and len(credentials) > 1:
                    password = credentials[1]
            else:
                host_port = host

            (host, tempport) = split_host_port(host_port, 8080)
            if port is None:
                port = tempport

            if host.endswith("/"):
                host = host[:-1]

        self.m_host = host
        self.m_port = port
        self.m_username = username
        self.m_password = password
        self.m_relay_id = relay_id

    def host(self):
        """
        host
        """
        return self.m_host

    def port(self):
        """
        port
        """
        return self.m_port

    def username(self):
        """
        username
        """
        return self.m_username

    def password(self):
        """
        password
        """
        return self.m_password

    def relay_id(self):
        """
        relay_id
        """
        return self.m_relay_id

    def __eq__(self, other):
        """
        __eq__
        """
        if isinstance(other, self.__class__):
            return self.__dict__ == other.__dict__
        return False

    def __ne__(self, other):
        """
        __ne__
        """
        return not self.__eq__(other)

    def auth_header(self):
        """
        auth_header
        """
        if self.m_username is None or self.m_password is None:
            return None

        raw = "%s:%s" % (self.m_username, self.m_password)
        return 'Basic ' + base64.b64encode(raw.encode('utf-8')).strip().decode('utf-8')


class InvalidCertificateException(http.client.HTTPException, urllib.error.URLError):
    """
    InvalidCertificateException
    """
    # pylint: disable=too-many-ancestors

    def __init__(self, host, cert, reason):
        """
        __init__
        """
        super(InvalidCertificateException, self).__init__()
        self.host = host
        self.cert = cert
        self.reason = reason

    def __str__(self):
        """
        __str__
        """
        return ('Host %s returned an invalid certificate (%s) %s\n' %
                (self.host, self.reason, self.cert))


class ProxyTunnelError(http.client.HTTPException):
    """
    ProxyTunnelError
    """

    def __init__(self, response):
        """
        __init__
        """
        super(ProxyTunnelError, self).__init__()
        self.response = response

    def close(self):
        """
        close
        """
        self.response.close()

    def __str__(self):
        """
        __str__
        """
        return "ProxyTunnelError(HTTPResponse(code=%d, reason=%s))" % (
            self.response.status, self.response.reason)


class CertValidatingHTTPSConnection(http.client.HTTPConnection):
    """
    CertValidatingHTTPSConnection
    """
    # pylint: disable=too-many-instance-attributes
    default_port = http.client.HTTPS_PORT

    def __init__(self, host, port=None, key_file=None, cert_file=None,
                 ca_certs=None, timeout=None, **kwargs):
        """
        __init__
        """
        # pylint: disable=too-many-arguments

        http.client.HTTPConnection.__init__(
            self, host, port, timeout, **kwargs)
        self.key_file = key_file
        self.cert_file = cert_file
        self.ca_certs = ca_certs
        self.auto_open = 0
        self.using_proxy = False
        if self.ca_certs:
            self.cert_reqs = ssl.CERT_REQUIRED
        else:
            self.cert_reqs = ssl.CERT_NONE
        if timeout:
            self.timeout = timeout

    def _tunnel(self):
        """
        Override the _tunnel method, so that we can convert 407 to an exception with a
        live response
        """
        connect = [
            "CONNECT %s:%d HTTP/1.0\r\n" %
            (self._tunnel_host, self._tunnel_port)]
        for header, value in self._tunnel_headers.items():
            connect.append("%s: %s\r\n" % (header, value))
        connect.append("\r\n")
        info( "Connect message: ", str(connect))
        content = ''.join(connect)
        self.send(content.encode('utf-8'))
        response = self.response_class(self.sock, method=self._method)
        response.begin()

        if response.version == 9:
            # HTTP/0.9 doesn't support the CONNECT verb, so if httplib has
            # concluded HTTP/0.9 is being used something has gone wrong.
            response.close()
            raise socket.error("Invalid response from tunnel request")
        elif response.status == 407:
            raise ProxyTunnelError(response)
        elif response.status != 200:
            response.close()
            raise socket.error(
                "Tunnel connection failed: %d %s" %
                (response.status, response.reason))

    def connect(self):
        "Connect to a host on a given (SSL) port."
        try:
            sock = socket.create_connection((self.host, self.port),
                                            self.timeout, self.source_address)
            if self._tunnel_host:
                self.sock = sock
                self._tunnel()
                hostname = self._tunnel_host
            else:
                hostname = self.host

            hostname = hostname.split(':', 0)[0]

            context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
            context.verify_mode = self.cert_reqs
            if self.cert_reqs in (ssl.CERT_REQUIRED, ssl.CERT_OPTIONAL):
                context.check_hostname = True
            if self.ca_certs is not None:
                context.load_verify_locations(cafile=self.ca_certs)

            self.sock = context.wrap_socket(sock,
                                            server_hostname=hostname)

            info("SSL Connection with %s cipher" % (str(self.sock.cipher())))

            # Cert verification now built-in (Python 2.7.9)
        except socket.error as exception:
            raise mcs_exception.MCSConnectionFailedException(exception)


class ConnectHTTPSHandler(urllib.request.HTTPSHandler):
    """
    ConnectHTTPSHandler
    """

    def do_open(self, http_class, req, **http_conn_args):
        return urllib.request.HTTPSHandler.do_open(
            self, CertValidatingHTTPSConnection, req, **http_conn_args)


def create_connection(
        url,
        proxy_url=None,
        cafile=None,
        debug=False,
        proxy_username=None,
        proxy_password=None):
    """
    create_connection
    """
    # pylint: disable=too-many-arguments, too-many-locals
    args = {}
    if cafile is not None:
        args["ca_certs"] = cafile

    proxy = Proxy(proxy_url, username=proxy_username, password=proxy_password)
    proxy_host = proxy.host()
    proxy_port = proxy.port()
    proxy_username = proxy.username()
    proxy_password = proxy.password()

    url_parsed = urllib.parse.urlparse(url)
    (url_host, url_port) = split_host_port(url_parsed.netloc, 443)

    auth = proxy_authorization.ProxyAuthorization(proxy, url_host, url_port)

    connection_class = CertValidatingHTTPSConnection

    retry = True
    retry_count = 0
    while retry and retry_count < 5:
        retry = False
        retry_count += 1

        if proxy_host:
            connection = connection_class(proxy_host, proxy_port, **args)

            # Get proxy header
            proxy_username_password = auth.auth_header()
            proxy_headers = set_proxy_headers(proxy_username_password)
            connection.set_tunnel(url_host, url_port, headers=proxy_headers)
            connection.using_proxy = True
        else:
            connection = connection_class(url_host, url_port, **args)
            connection.using_proxy = False

        if debug:
            connection.set_debuglevel(2)

        try:
            connection.connect()

        except ProxyTunnelError as exception:
            assert proxy_host is not None
            # If basic failed, try a different scheme (if supported)
            retry = auth.update_auth_header(exception.response)
            if not retry:
                raise
            exception.close()

    return connection

def set_proxy_headers(proxy_username_password):
    """
    set_proxy_headers
    """
    if proxy_username_password is None:
        proxy_headers = None
    else:
        proxy_headers = {
            'Proxy-authorization': proxy_username_password
        }
    return proxy_headers

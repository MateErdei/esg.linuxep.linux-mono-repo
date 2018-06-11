#!/usr/bin/env python

from __future__ import print_function,division,unicode_literals

import socket
socket.ssl
import _ssl
import ssl
import httplib
httplib.HTTPS

import os
import re
import sys
import urllib2
import base64

## urllib.parse in python 3
import urlparse

import ProxyAuthorization

import logging
logger = None

def info(*message):
    if logger is None:
        return
    logger.info(*message)

def splitHostPort(hostport,defaultPort=80):
    """
    Split a hostname:port into (hostname,port) with a default port
    """
    s = hostport.rsplit(":",1)
    host = s[0]
    if len(s) == 2:
        if s[1].endswith("/"):
            s[1] = s[1][:-1]
        try:
            port = int(s[1])
        except ValueError:
            port = defaultPort
    else:
        port = defaultPort
    return (host,port)

def getProxy(proxy=None):
    if not proxy:
        proxy = os.environ.get("https_proxy",None)
    if proxy == "noproxy:":
        proxyHost = None
        proxyPort = None
    elif proxy is not None:
        if proxy.startswith("http://"):
            proxy = proxy[len("http://"):]
        if proxy.endswith("/"):
            proxy = proxy[:-1]
        (proxyHost,proxyPort) = splitHostPort(proxy,8080)
    else:
        proxyHost = None
        proxyPort = None

    return (proxyHost,proxyPort)

class Proxy(object):
    def __init__(self, host=None, port=None, username=None, password=None, relayId=None):
        if host in (None,"noproxy:",""):
            ## If we don't have a host then we don't have a proxy at all
            host = None
            port = None
            username = None
            password = None
            relayId = None
        else:
            if host.startswith("http://"):
                host = host[len("http://"):]

            if host.startswith("https://"):
                host = host[len("https://"):]

            if "@" in host:
                credentials,hostport = host.rsplit("@",1)
                credentials = credentials.split(":",1)

                if username is None:
                    username = credentials[0]
                if password is None and len(credentials) > 1:
                    password = credentials[1]
            else:
                hostport = host

            (host,tempport) = splitHostPort(hostport, 8080)
            if port is None:
                port = tempport

            if host.endswith("/"):
                host = host[:-1]

        self.m_host = host
        self.m_port = port
        self.m_username = username
        self.m_password = password
        self.m_relayId = relayId

    def host(self):
        return self.m_host

    def port(self):
        return self.m_port

    def username(self):
        return self.m_username

    def password(self):
        return self.m_password

    def relayId(self):
        return self.m_relayId

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.__dict__ == other.__dict__
        else:
            return False

    def __ne__(self, other):
        return not self.__eq__(other)

    def authHeader(self):
        if self.m_username is None or self.m_password is None:
            return None

        raw = "%s:%s" % (self.m_username, self.m_password)
        return 'Basic %s' % base64.b64encode(raw).strip()

class InvalidCertificateException(httplib.HTTPException, urllib2.URLError):
    def __init__(self, host, cert, reason):
        httplib.HTTPException.__init__(self)
        self.host = host
        self.cert = cert
        self.reason = reason

    def __str__(self):
        return ('Host %s returned an invalid certificate (%s) %s\n' %
                (self.host, self.reason, self.cert))

class ProxyTunnelError(httplib.HTTPException):
    def __init__(self, response):
        self.response = response

    def close(self):
        self.response.close()

    def __str__(self):
        return "ProxyTunnelError(HTTPResponse(code=%d, reason=%s))" % (
                self.response.status, self.response.reason)

class CertValidatingHTTPSConnection(httplib.HTTPConnection):
    default_port = httplib.HTTPS_PORT

    def __init__(self, host, port=None, key_file=None, cert_file=None,
                             ca_certs=None, strict=None, timeout=None, **kwargs):
        httplib.HTTPConnection.__init__(self, host, port, strict, timeout, **kwargs)
        self.key_file = key_file
        self.cert_file = cert_file
        self.ca_certs = ca_certs
        self.auto_open = 0
        if self.ca_certs:
            self.cert_reqs = ssl.CERT_REQUIRED
        else:
            self.cert_reqs = ssl.CERT_NONE
        if timeout:
            self.timeout = timeout

    _MAXLINE = httplib._MAXLINE

    def _tunnel(self):
        """
        Override the _tunnel method, so that we can convert 407 to an exception with a
        live response
        """
        connect = ["CONNECT %s:%d HTTP/1.0\r\n" % (self._tunnel_host, self._tunnel_port)]
        for header, value in self._tunnel_headers.iteritems():
            connect.append("%s: %s\r\n" % (header, value))
        connect.append("\r\n")
        self.send("".join(connect))
        response = self.response_class(self.sock, strict = self.strict,
                                       method = self._method)
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
            raise socket.error("Tunnel connection failed: %d %s" % (response.status,
                                                                    response.reason))

    def connect(self):
        "Connect to a host on a given (SSL) port."
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
        if self.cert_reqs in ( ssl.CERT_REQUIRED, ssl.CERT_OPTIONAL ):
            context.check_hostname = True
        if self.ca_certs is not None:
            context.load_verify_locations(cafile=self.ca_certs)

        self.sock = context.wrap_socket(sock,
            server_hostname=hostname)

        info("SSL Connection with %s cipher"%(str(self.sock.cipher())))

        ## Cert verification now built-in (Python 2.7.9)

class ConnectHTTPSHandler(urllib2.HTTPSHandler):
    def do_open(self, http_class, req):
        return urllib2.HTTPSHandler.do_open(self, CertValidatingHTTPSConnection, req)

def createConnection(url,proxyurl=None,cafile=None,debug=False,proxyUsername=None,proxyPassword=None):
    args = {}
    if cafile is not None:
        args["ca_certs"] = cafile

    proxy = Proxy(proxyurl, username=proxyUsername, password=proxyPassword)
    proxyHost = proxy.host()
    proxyPort = proxy.port()
    proxyUsername = proxy.username()
    proxyPassword = proxy.password()

    url_parsed = urlparse.urlparse(url)
    (url_host,url_port) = splitHostPort(url_parsed.netloc,443)

    auth = ProxyAuthorization.ProxyAuthorization(proxy, url_host, url_port)

    ConnectionClass = CertValidatingHTTPSConnection

    #~ print proxyurl, proxyHost, proxyPort
    retry = True
    retryCount = 0
    while retry and retryCount < 5:
        retry = False
        retryCount += 1

        if proxyHost:
            connection = ConnectionClass(proxyHost,proxyPort,**args)

            # Get proxy header
            proxyUsernamePassword = auth.authHeader()
            if proxyUsernamePassword is None:
                proxyHeaders = None
            else:
                proxyHeaders = {
                    'Proxy-authorization' : proxyUsernamePassword
                }
            connection.set_tunnel(url_host,url_port,headers=proxyHeaders)
            connection.using_proxy = True
        else:
            connection = ConnectionClass(url_host,url_port,**args)
            connection.using_proxy = False

        if debug:
            connection.set_debuglevel(2)

        try:
            connection.connect()

        except ProxyTunnelError as e:
            assert proxyHost is not None
            # If basic failed, try a different scheme (if supported)
            retry = auth.updateAuthHeader(e.response)
            if not retry:
                raise
            e.close()

    return connection

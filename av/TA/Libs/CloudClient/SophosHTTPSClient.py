#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019 Sophos Ltd
# All rights reserved.
from __future__ import absolute_import, print_function, division, unicode_literals

import six
from six.moves.http_cookiejar import LWPCookieJar

import sys
import os
import json
import ssl
import socket
import logging


try:
    from . import SophosRobotSupport
except ImportError:
    import SophosRobotSupport

try:
    # for Python 3
    import urllib.parse as urlparse
except ImportError:
    # for Python 2
    import urlparse


try:
    from robot.api import logger
    logger.warning = logger.warn
except ImportError:
    logger = logging.getLogger(__name__)

log = logging.getLogger('Patch')

PY2 = sys.version_info[0] == 2

PY2_7_9_PLUS = (sys.version_info[0] == 2 and
                (sys.version_info[1] > 7
                 or
                 (sys.version_info[1] == 7 and sys.version_info[2] >= 9)))

PY3 = sys.version_info[0] >= 3

VERBOSE = False

DEFAULT_CIPHERS = (
    'ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:'
    'ECDH+HIGH:DH+HIGH:ECDH+3DES:DH+3DES:RSA+AESGCM:RSA+AES:RSA+HIGH:'
    'RSA+3DES:ECDH+RC4:DH+RC4:RSA+RC4:-aNULL:-eNULL:-EXP:-MD5:RSA+RC4+MD5'
)


socket.setdefaulttimeout(60)

if PY3:
    import urllib
else:
    from six.moves import urllib

try:
    from builtins import str
except ImportError:
    pass


def wrap_ssl_socket(hostname=None):
    ## monkey patch to prevent sslv2 usage

    from functools import wraps
    def sslwrap(func, hostname=None):
        @wraps(func)
        def bar(*args, **kw):
            print("in sslwrap() host=", hostname, file=sys.stderr)
            if PY2_7_9_PLUS:
                kw['ssl_version'] = ssl.PROTOCOL_TLSv1_2
            else:
                kw['ssl_version'] = ssl.PROTOCOL_TLSv1
            # ~ print
            # ~ kw['server_hostname'] = hostname
            return func(*args, **kw)

        return bar

    # ~ print("wrap_ssl_socket hostname=%s"%str(hostname), file=sys.stderr)
    ssl.wrap_socket = sslwrap(ssl.wrap_socket, hostname)

    if PY2:
        ssl._create_default_https_context = ssl._create_unverified_context


def host(hostname=None):
    if hostname is None:
        hostname = os.uname()[1]

    return hostname.split(".")[0]


cloud_urllib2_opener = None
URLLIB2_HANDLERS = []


def _add_handler(handler):
    global URLLIB2_HANDLERS
    global cloud_urllib2_opener

    URLLIB2_HANDLERS.append(handler)
    # ~ handlers.sort(key=lambda h: h.handler_order)
    URLLIB2_HANDLERS.sort()
    cloud_urllib2_opener = six.moves.urllib.request.build_opener(*URLLIB2_HANDLERS)
    six.moves.urllib.request.install_opener(cloud_urllib2_opener)
    return cloud_urllib2_opener


class SophosPasswordManager(six.moves.urllib.request.HTTPPasswordMgr):
    def setUserPassword(self, user, password):
        self.m_user = user
        self.m_password = password

    def find_user_password(self, realm, authuri):
        return self.m_user, self.m_password


class SophosProxyDigestAuthHandler(six.moves.urllib.request.BaseHandler,
                                   six.moves.urllib.request.AbstractDigestAuthHandler):
    auth_header = 'Proxy-Authorization'
    handler_order = 490  # before Basic auth

    def http_error_407(self, req, fp, code, msg, headers):
        # ~ logger.debug("DIGEST http_error_407 %d",code)
        host = req.get_host()
        retry = self.http_error_auth_reqed('proxy-authenticate',
                                           host, req, headers)
        self.reset_retry_count()
        return retry

    def get_authorization(self, req, chal):
        try:
            realm = chal['realm']
            nonce = chal['nonce']
            qop = chal.get('qop')
            algorithm = chal.get('algorithm', 'MD5')
            # mod_digest doesn't send an opaque, even though it isn't
            # supposed to be optional
            opaque = chal.get('opaque', None)
        except KeyError:
            return None

        H, KD = self.get_algorithm_impls(algorithm)
        if H is None:
            return None

        user, pw = self.passwd.find_user_password(realm, req.get_full_url())
        if user is None:
            return None

        # XXX not implemented yet
        if req.has_data():
            entdig = self.get_entity_digest(req.get_data(), chal)
        else:
            entdig = None

        A1 = "%s:%s:%s" % (user, realm, pw)
        if req._tunnel_host:
            # All requests with _tunnel_host are https; hardwire 443
            uri = "%s:443" % req._tunnel_host
            A2 = "CONNECT:%s" % uri
        else:
            # XXX selector: what about full urls
            uri = req.get_selector()
            A2 = "%s:%s" % (req.get_method(), uri)
        if qop == 'auth':
            if nonce == self.last_nonce:
                self.nonce_count += 1
            else:
                self.nonce_count = 1
                self.last_nonce = nonce

            ncvalue = '%08x' % self.nonce_count
            cnonce = self.get_cnonce(nonce)
            noncebit = "%s:%s:%s:%s:%s" % (nonce, ncvalue, cnonce, qop, H(A2))
            respdig = KD(H(A1), noncebit)
        elif qop is None:
            respdig = KD(H(A1), "%s:%s" % (nonce, H(A2)))
        else:
            # XXX handle auth-int.
            raise six.moves.urllib.error.URLError("qop '%s' is not supported." % qop)

        # XXX should the partial digests be encoded too?

        base = 'username="%s", realm="%s", nonce="%s", uri="%s", ' \
               'response="%s"' % (user, realm, nonce, uri, respdig)
        if opaque:
            base += ', opaque="%s"' % opaque
        if entdig:
            base += ', digest="%s"' % entdig
        base += ', algorithm="%s"' % algorithm
        if qop:
            base += ', qop=auth, nc=%s, cnonce="%s"' % (ncvalue, cnonce)
        return base


class SophosProxyBasicAuthHandler(six.moves.urllib.request.AbstractBasicAuthHandler,
                                  six.moves.urllib.request.BaseHandler):
    auth_header = 'Proxy-authorization'

    def http_error_407(self, req, fp, code, msg, headers):
        # ~ logger.debug("BASIC http_error_407 %d",code)
        # http_error_auth_reqed requires that there is no userinfo component in
        # authority.  Assume there isn't one, since urllib2 does not (and
        # should not, RFC 3986 s. 3.2.1) support requests for URLs containing
        # userinfo.
        authority = req.get_host()
        response = self.http_error_auth_reqed('proxy-authenticate',
                                              authority, req, headers)
        return response


class ProxyTunnelError(six.moves.http_client.HTTPException):
    def __init__(self, response):
        self.response = response

    def __str__(self):
        return "ProxyTunnelError(HTTPResponse(code=%d, reason=%s))" % (
            self.response.status, self.response.reason)


def set_proxy(proxy=None, username=None, password=None):
    if proxy == "":
        proxy = None

    if proxy is None:
        proxy = os.environ.get('https_proxy', None)

    if proxy is None:
        return

    logger.debug('proxy: %s', proxy)
    _add_handler(six.moves.urllib.request.ProxyHandler({"http": proxy, "https": proxy}))

    if username is None:
        parts = six.moves.urllib.parse.urlparse(proxy)
        # ~ logger.critical("%s",parts.netloc)
        if "@" in parts.netloc:
            # ~ logger.critical("2 %s",parts.netloc)
            usernamepassword = parts.netloc.split("@", 1)[0]
            # ~ logger.critical("%s",usernamepassword)
            if ":" in usernamepassword:
                # ~ logger.critical("2 %s",usernamepassword)
                username, password = usernamepassword.split(":", 1)
            else:
                username = usernamepassword
                password = ""

    if username is not None:
        logger.debug("Using proxy authentication: %s password=%s", username, password)
        passwordManager = SophosPasswordManager()
        passwordManager.setUserPassword(username, password)
        _add_handler(SophosProxyBasicAuthHandler(passwordManager))
        cloud_urllib2_opener = _add_handler(SophosProxyDigestAuthHandler(passwordManager))
        # ~ logger.debug("%s",str(cloud_urllib2_opener))
        # ~ logger.debug("%s",str(cloud_urllib2_opener.handlers))


class SophosSSLContext(ssl.SSLContext):
    def wrap_socket(self, sock, server_side=False,
                    do_handshake_on_connect=True,
                    suppress_ragged_eofs=True,
                    server_hostname=None):
        # ~ print("SophosSSLContext.wrap_socket sock=%s server_side=%s \n do_handshake_on_connect=%s suppress_ragged_eofs=%s server_hostname=%s"%(
        # ~ str(sock),
        # ~ str(server_side),
        # ~ str(do_handshake_on_connect),
        # ~ str(suppress_ragged_eofs),
        # ~ str(server_hostname)
        # ~ ), file=sys.stderr)
        # ~ import pdb; pdb.set_trace()
        s = super(SophosSSLContext, self).wrap_socket(sock, server_side,
                                                      False,
                                                      suppress_ragged_eofs,
                                                      server_hostname)
        if do_handshake_on_connect:
            try:
                s.do_handshake()
            except Exception as e:
                print("Exception=", e, type(e), file=sys.stderr)
                raise
        return s


G_SSL_CONTEXT = SophosSSLContext(ssl.PROTOCOL_TLSv1)


def getDefaultCAfile():
    TMPROOT = SophosRobotSupport.TMPROOT()
    PLATFORM = os.environ.get("PLATFORM", "linux")

    DISTDIR = os.environ.get("DISTDIR", TMPROOT + "/sophos-av")
    platform = PLATFORM
    ca_file = os.path.join(DISTDIR, "sav-%s" % (platform), "common", "engine", "mcs_rootca.crt")
    if os.path.isfile(ca_file):
        return ca_file
    return None


def loadCA(context):
    context.load_default_certs()
    ca_file = os.environ.get("CAFILE", getDefaultCAfile())
    if ca_file is not None and os.path.isfile(ca_file):
        context.load_verify_locations(cafile=ca_file)
        if VERBOSE:
            print("Loading ca_file=%s" % ca_file, file=sys.stderr)
        return True
    else:
        print("Not loading ca_file=%s" % ca_file, file=sys.stderr)
        return False


class CloudHTTPSHandler(six.moves.urllib.request.HTTPSHandler):
    def https_open(self, req):
        # ~ print("CloudHTTPSHandler.https_open(%s)"%str(req),file=sys.stderr)
        try:
            return self.do_open(six.moves.http_client.HTTPSConnection, req)
        except ProxyTunnelError as e:
            # ~ logger.exception("ProxyTunnelError %s",str(e.response.status))
            if e.response.status == 407:
                fp = socket._fileobject(e.response, close=True)
                resp = six.moves.urllib.response.addinfourl(fp,
                                                            e.response.msg,
                                                            req.get_full_url())
                return self.parent.error('https', req, resp,
                                         e.response.status, e.response.reason,
                                         resp.info())
            else:
                raise six.moves.urllib.error.HTTPError(req.get_full_url(),
                                                       e.response.status, e.response.reason,
                                                       e.response.msg, e.response.fp)


##########################################################################################

if PY2:

    _MAXLINE = six.moves.http_client._MAXLINE


    def _tunnel(self):
        connect = ["CONNECT %s:%d HTTP/1.0\r\n" % (self._tunnel_host, self._tunnel_port)]
        for header, value in six.iteritems(self._tunnel_headers):
            connect.append("%s: %s\r\n" % (header, value))
        connect.append("\r\n")
        self.send("".join(connect))
        response = self.response_class(self.sock, strict=self.strict,
                                       method=self._method)

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


    six.moves.http_client.HTTPConnection._tunnel = _tunnel

    # ~ pass


##########################################################################################
##########################################################################################


def set_ssl_context(verify=True):
    global G_SSL_CONTEXT
    context = G_SSL_CONTEXT
    context.set_ciphers(DEFAULT_CIPHERS)  ## Allow MD5

    if ssl.HAS_SNI and verify:
        context.verify_mode = ssl.CERT_REQUIRED
        # ~ context.verify_mode = ssl.CERT_OPTIONAL
        context.check_hostname = True

    else:
        print("Disabling SNI!!!", file=sys.stderr)
        context.verify_mode = ssl.CERT_NONE
        context.check_hostname = False

    if not loadCA(context):
        print("Disabling certificate verification!", file=sys.stderr)
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE

    # ~ print("context=%s"%(str(dir(context))), file=sys.stderr)
    ##  debuglevel=2,
    _add_handler(CloudHTTPSHandler(context=context))


def getRequestData(request):
    try:
        return request.data
    except AttributeError:
        return request.get_data()


def json_loads(text):
    assert text is not None, "Trying to load None as Json"

    try:
        return json.loads(text)
    except ValueError as e:
        print("Failed to decode text as json: " + str(e), file=sys.stderr)
        print("Head to text:" + text[:1000], file=sys.stderr)
        print("Tail to text:" + text[-1000:], file=sys.stderr)
        raise
    except TypeError as e:
        print("Failed to decode text as json: " + str(e), file=sys.stderr)
        raise


GL_COOKIE_JAR = None


def getCookies():
    global GL_COOKIE_JAR

    TMPROOT = SophosRobotSupport.TMPROOT()
    GL_COOKIE_JAR = LWPCookieJar(os.path.join(TMPROOT, "cloud_cookies.txt"))
    try:
        GL_COOKIE_JAR.load(ignore_discard=True)
    except IOError:
        # no cookie file, create a new one
        GL_COOKIE_JAR.save()
    _add_handler(six.moves.urllib.request.HTTPCookieProcessor(GL_COOKIE_JAR))
    logger.debug("Added cookie handler")


getCookies()


def raw_request_url(request, verbose=True):
    try:
        if VERBOSE or verbose:
            logger.debug("{method}ing {url}".format(url=request.get_full_url(), method=request.get_method()))
            # ~ print("%s %s"%(str(request),str(dir(request))),file=sys.stderr)
            # ~ GL_COOKIE_JAR.add_cookie_header(request) ## include a Cookie: header in the dump
            # ~ for (name,value) in request.header_items():
            # ~ print("\t%s : %s"%(name,value),file=sys.stderr)
        data = getRequestData(request)
        if data is not None:
            try:
                # pretty print the data, if it's actually JSON
                data = json.dumps(json_loads(str(data.decode('UTF-8'))), indent=2, sort_keys=True)
            except ValueError:
                pass
            if VERBOSE or verbose:
                logger.debug("data: {}".format(data))
        response = cloud_urllib2_opener.open(request)
        return response
    except six.moves.urllib.error.HTTPError as e:
        body = e.read()
        logger.error('HTTP Error {code}: {reason}'.format(code=e.code, reason=e.reason))
        logger.error("Body: {body}".format(body=body))
        e.body = body
        raise
    except six.moves.urllib.error.URLError as e:
        logger.error('URL Error {reason}'.format(reason=e.reason))
        raise
    except ssl.SSLError as e:
        logger.error('SSL Error {reason}'.format(reason=str(e)))
        raise
    except socket.error as e:
        logger.error('Socket Error {reason}'.format(reason=str(e)))
        raise
    except EnvironmentError as e:
        logger.error('EnvironmentError {reason}'.format(reason=str(e)))
        raise
    finally:
        ## can we get updated cookies from a 4xx response?
        GL_COOKIE_JAR.save(ignore_discard=True)


def request_url(request, verbose=True):
    response = raw_request_url(request, verbose)
    body = response.read().decode('UTF-8')
    return body


def main(argv):
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

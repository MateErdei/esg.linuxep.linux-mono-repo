#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Derived from Perforce //dev/savlinux/regression/supportFiles/CloudAutomation/SophosHTTPSClient.py



import sys

VERBOSE=False

DEFAULT_CIPHERS = (
 'ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:'
 'ECDH+HIGH:DH+HIGH:ECDH+3DES:DH+3DES:RSA+AESGCM:RSA+AES:RSA+HIGH:'
 'RSA+3DES:ECDH+RC4:DH+RC4:RSA+RC4:-aNULL:-eNULL:-EXP:-MD5:RSA+RC4+MD5'
 )

from urllib.error import URLError

import os
import json
import ssl
import socket
import urllib.parse
import http
import logging
logger = logging.getLogger(__name__)
log = logging.getLogger('Patch')

socket.setdefaulttimeout(60)

import urllib.request, urllib.error, urllib.parse
import http.client
from http.cookiejar import LWPCookieJar

def wrap_ssl_socket(hostname=None):
    ## monkey patch to prevent sslv2 usage

    from functools import wraps
    def sslwrap(func, hostname=None):
        @wraps(func)
        def bar(*args, **kw):
            kw['ssl_version'] = ssl.PROTOCOL_TLSv1

            return func(*args, **kw)
        return bar

    ssl.wrap_socket = sslwrap(ssl.wrap_socket, hostname)
    ssl._create_default_https_context = ssl._create_unverified_context

def host(hostname=None):
    if hostname is None:
        hostname = os.uname()[1]

    return hostname.split(".")[0]

cloud_urllib2_opener = None
URLLIB2_HANDLERS=[]

def _generate_opener():
    global URLLIB2_HANDLERS
    global cloud_urllib2_opener

    URLLIB2_HANDLERS.sort()
    cloud_urllib2_opener = urllib.request.build_opener(*URLLIB2_HANDLERS)
    urllib.request.install_opener(cloud_urllib2_opener)
    return cloud_urllib2_opener


def remove_proxy_handlers():
    global URLLIB2_HANDLERS

    temp = []
    for handler in URLLIB2_HANDLERS:
        if isinstance(handler, urllib.request.ProxyHandler):
            continue
        if isinstance(handler, SophosProxyBasicAuthHandler):
            continue
        if isinstance(handler, SophosProxyDigestAuthHandler):
            continue

        temp.append(handler)

    URLLIB2_HANDLERS = temp
    return _generate_opener()


def _add_handler(handler):
    global URLLIB2_HANDLERS
    global cloud_urllib2_opener

    URLLIB2_HANDLERS.append( handler )

    return _generate_opener()

class SophosPasswordManager(urllib.request.HTTPPasswordMgr):
    def setUserPassword(self, user, password):
        self.m_user = user
        self.m_password = password

    def find_user_password(self, realm, authuri):
        return self.m_user,self.m_password

class SophosProxyDigestAuthHandler(urllib.request.BaseHandler, urllib.request.AbstractDigestAuthHandler):

    auth_header = 'Proxy-Authorization'
    handler_order = 490  # before Basic auth

    def http_error_407(self, req, fp, code, msg, headers):
        #~ logger.debug("DIGEST http_error_407 %d",code)
        host = req.get_host()
        retry = self.http_error_auth_reqed('proxy-authenticate',
                                           host, req, headers)
        self.reset_retry_count()
        return retry

    def get_authorization(self, req, chal):
        ncvalue = None
        cnonce = None
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
            raise URLError("qop '%s' is not supported." % qop)

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

class SophosProxyBasicAuthHandler(urllib.request.AbstractBasicAuthHandler, urllib.request.BaseHandler):

    auth_header = 'Proxy-authorization'

    def http_error_407(self, req, fp, code, msg, headers):
        #~ logger.debug("BASIC http_error_407 %d",code)
        # http_error_auth_reqed requires that there is no userinfo component in
        # authority.  Assume there isn't one, since urllib2 does not (and
        # should not, RFC 3986 s. 3.2.1) support requests for URLs containing
        # userinfo.
        authority = req.get_host()
        response = self.http_error_auth_reqed('proxy-authenticate',
                                          authority, req, headers)
        return response


class ProxyTunnelError(http.client.HTTPException):
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
        # Remove proxy handlers
        return remove_proxy_handlers()

    logger.debug('proxy: %s',proxy)
    cloud_urllib2_opener = _add_handler( urllib.request.ProxyHandler({"http":proxy, "https":proxy}) )

    if username is None:
        parts = urllib.parse.urlparse(proxy)
        #~ logger.critical("%s",parts.netloc)
        if "@" in parts.netloc:
            #~ logger.critical("2 %s",parts.netloc)
            usernamepassword = parts.netloc.split("@",1)[0]
            #~ logger.critical("%s",usernamepassword)
            if ":" in usernamepassword:
                #~ logger.critical("2 %s",usernamepassword)
                username,password = usernamepassword.split(":",1)
            else:
                username = usernamepassword
                password = ""

    if username is not None:
        logger.debug("Using proxy authentication: %s password=%s",username,password)
        passwordManager = SophosPasswordManager()
        passwordManager.setUserPassword(username,password)
        _add_handler( SophosProxyBasicAuthHandler(passwordManager) )
        cloud_urllib2_opener = _add_handler( SophosProxyDigestAuthHandler(passwordManager) )
        #~ logger.debug("%s",str(cloud_urllib2_opener))
        #~ logger.debug("%s",str(cloud_urllib2_opener.handlers))

    return cloud_urllib2_opener


class SophosSSLContext(ssl.SSLContext):
    def wrap_socket(self, sock, server_side=False,
                    do_handshake_on_connect=True,
                    suppress_ragged_eofs=True,
                    server_hostname=None):
        #~ print("SophosSSLContext.wrap_socket sock=%s server_side=%s \n do_handshake_on_connect=%s suppress_ragged_eofs=%s server_hostname=%s"%(
                #~ str(sock),
                #~ str(server_side),
                #~ str(do_handshake_on_connect),
                #~ str(suppress_ragged_eofs),
                #~ str(server_hostname)
            #~ ), file=sys.stderr)
        #~ import pdb; pdb.set_trace()
        s = super(SophosSSLContext,self).wrap_socket(sock, server_side,
                    False,
                    suppress_ragged_eofs,
                    server_hostname)
        if do_handshake_on_connect:
            try:
                s.do_handshake()
            except Exception as e:
                print("Exception=",e,type(e),file=sys.stderr)
                raise
        return s

G_SSL_CONTEXT = SophosSSLContext(ssl.PROTOCOL_TLSv1)

def getDefaultCAfile():
    TMPROOT=os.environ.get("TMPROOT","/tmp/PeanutIntegration")
    PLATFORM=os.environ.get("PLATFORM","linux")

    DISTDIR=os.environ.get("DISTDIR",TMPROOT+"/sophos-av")
    platform = PLATFORM
    ca_file = os.path.join(DISTDIR,"sav-%s"%(platform),"common", "engine","mcs_rootca.crt")
    if os.path.isfile(ca_file):
        return ca_file
    return None

def loadCA(context):
    context.load_default_certs()
    ca_file = os.environ.get("CAFILE",getDefaultCAfile())
    if ca_file is not None and os.path.isfile(ca_file):
        context.load_verify_locations(cafile=ca_file)
        if VERBOSE:
            print("Loading ca_file=%s"%ca_file, file=sys.stderr)
        return True
    else:
        print("Not loading ca_file=%s"%ca_file, file=sys.stderr)
        return False

class CloudHTTPSHandler(urllib.request.HTTPSHandler):
    def https_open(self, req):
        #~ print("CloudHTTPSHandler.https_open(%s)"%str(req),file=sys.stderr)
        try:
            return self.do_open(http.client.HTTPSConnection, req)
        except ProxyTunnelError as e:
            #~ logger.exception("ProxyTunnelError %s",str(e.response.status))
            if e.response.status == 407:
                fp = socket._fileobject(e.response, close=True)
                resp = urllib.addinfourl(fp,
                                         e.response.msg,
                                         req.get_full_url())
                return self.parent.error('https', req, resp,
                        e.response.status, e.response.reason,
                        resp.info())
            else:
                raise urllib.error.HTTPError(req.get_full_url(),
                        e.response.status, e.response.reason,
                        e.response.msg, e.response.fp)


def set_ssl_context(verify=True):
    global G_SSL_CONTEXT
    context = G_SSL_CONTEXT
    context.set_ciphers(DEFAULT_CIPHERS) ## Allow MD5

    if ssl.HAS_SNI and verify:
        context.verify_mode = ssl.CERT_REQUIRED
        #~ context.verify_mode = ssl.CERT_OPTIONAL
        context.check_hostname = True

    else:
        print("Disabling SNI!!!", file=sys.stderr)
        context.verify_mode = ssl.CERT_NONE
        context.check_hostname = False

    if not loadCA(context):
        print("Disabling certificate verification!", file=sys.stderr)
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE

    #~ print("context=%s"%(str(dir(context))), file=sys.stderr)
    ##  debuglevel=2,
    _add_handler( CloudHTTPSHandler(context=context ) )

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
        print("Failed to decode text as json: "+str(e),file=sys.stderr)
        print("Head to text:"+text[:1000],file=sys.stderr)
        print("Tail to text:"+text[-1000:],file=sys.stderr)
        raise
    except TypeError as e:
        print("Failed to decode text as json: "+str(e),file=sys.stderr)
        raise

GL_COOKIE_JAR = None

def getCookies():
    global GL_COOKIE_JAR

    TMPROOT=os.environ.get("TMPROOT", "/tmp")
    GL_COOKIE_JAR = LWPCookieJar(os.path.join(TMPROOT,"cloud_cookies.txt"))
    try:
        GL_COOKIE_JAR.load(ignore_discard=True)
    except IOError:
        # no cookie file, create a new one
        GL_COOKIE_JAR.save()
    _add_handler(urllib.request.HTTPCookieProcessor(GL_COOKIE_JAR))

getCookies()

def raw_request_url(request, verbose=True):
    try:
        if VERBOSE or verbose:
            print("{method}ing {url}".format(url=request.get_full_url(), method=request.get_method()),file=sys.stderr)
            #~ print("%s %s"%(str(request),str(dir(request))),file=sys.stderr)
            #~ GL_COOKIE_JAR.add_cookie_header(request) ## include a Cookie: header in the dump
            #~ for (name,value) in request.header_items():
                #~ print("\t%s : %s"%(name,value),file=sys.stderr)
        data = getRequestData(request)
        if data is not None:
            try:
                # pretty print the data, if it's actually JSON
                data = json.dumps(json_loads(str(data.decode('UTF-8'))), indent=2, sort_keys=True)
            except ValueError:
                pass
            if VERBOSE or verbose:
                print("data: {}".format(data),file=sys.stderr)
        response = cloud_urllib2_opener.open(request)
        return response
    except urllib.error.HTTPError as e:
        body = e.read()
        print('HTTP Error {code}: {reason}'.format(code=e.code, reason=e.reason),file=sys.stderr)
        print("Body: {body}".format(body=body),file=sys.stderr)
        e.body = body
        raise
    except urllib.error.URLError as e:
        print('URL Error {reason}'.format(reason=e.reason), file=sys.stderr)
        raise
    except ssl.SSLError as e:
        print('SSL Error {reason}'.format(reason=str(e)), file=sys.stderr)
        raise
    except socket.error as e:
        print('Socket Error {reason}'.format(reason=str(e)), file=sys.stderr)
        raise
    except EnvironmentError as e:
        print('EnvironmentError {reason}'.format(reason=str(e)), file=sys.stderr)
        raise
    finally:
        ## can we get updated cookies from a 4xx response?
        GL_COOKIE_JAR.save(ignore_discard=True)

def request_url(request, verbose=True):
    response = raw_request_url(request, verbose)
    return response.read().decode('UTF-8')

def main(argv):
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))

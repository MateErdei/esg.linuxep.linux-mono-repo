#!/bin/env python
# -*- coding: utf-8 -*-

## From https://github.com/inaz2/proxy2
## Licence: BSD 3-clause "New" or "Revised" License

#~ Copyright (c) 2015, inaz2
#~ All rights reserved.

#~ Redistribution and use in source and binary forms, with or without
#~ modification, are permitted provided that the following conditions are met:

#~ * Redistributions of source code must retain the above copyright notice, this
  #~ list of conditions and the following disclaimer.

#~ * Redistributions in binary form must reproduce the above copyright notice,
  #~ this list of conditions and the following disclaimer in the documentation
  #~ and/or other materials provided with the distribution.

#~ * Neither the name of proxy2 nor the names of its
  #~ contributors may be used to endorse or promote products derived from
  #~ this software without specific prior written permission.

#~ THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#~ AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#~ IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#~ DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#~ FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#~ DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#~ SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#~ CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#~ OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#~ OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


import sys
import os
import socket
import ssl
import select
import http.client
import urllib.parse
import threading
import gzip
import zlib
import time
import json
import re
import optparse
from http.server import HTTPServer, BaseHTTPRequestHandler
from socketserver import ThreadingMixIn
from io import StringIO
from subprocess import Popen, PIPE
from html.parser import HTMLParser

import logging
logger = logging.getLogger("http_proxy")

import https_proxy_lib.HTTPAuthentication

def with_color(c, s):
    return "\x1b[%dm%s\x1b[0m" % (c, s)

def join_with_script_dir(path):
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), path)


class ThreadingHTTPServer(ThreadingMixIn, HTTPServer):
    address_family = socket.AF_INET
    daemon_threads = True

    def handle_error(self, request, client_address):
        # surpress socket/ssl related errors
        cls, e = sys.exc_info()[:2]
        if cls is socket.error or cls is ssl.SSLError:
            pass
        else:
            return HTTPServer.handle_error(self, request, client_address)

GL_AUTHENTICATION = https_proxy_lib.HTTPAuthentication.NullAuthentication()

class ProxyRequestHandler(BaseHTTPRequestHandler):
    cakey = join_with_script_dir('ca.key')
    cacert = join_with_script_dir('ca.crt')
    certkey = join_with_script_dir('cert.key')
    certdir = join_with_script_dir('certs/')
    timeout = 5
    lock = threading.Lock()
    ALLOW_RELAY = True
    TIME_OUT_CONNECTION = False
    DUMMY_HOST = True
    PARENT_PROXY = None
    OPTIONS = None

    def __init__(self, *args, **kwargs):
        self.tls = threading.local()
        self.tls.conns = {}

        BaseHTTPRequestHandler.__init__(self, *args, **kwargs)

    def dummyReplacement(self, host):
        if ProxyRequestHandler.DUMMY_HOST:
           host = host.replace("dummyhost","localhost")
           host = host.replace("dummydci","dci.sophosupd.com")
           host = host.replace("dummysandbox","mcs.sandbox.sophos")
        return host

    def log_error(self, format, *args):
        # surpress "Request timed out: timeout('timed out',)"
        if isinstance(args[0], socket.timeout):
            return

        self.log_message(format, *args)

    def send_407(self, message="Proxy Authentication Required"):
        self.send_response(407, message)


    def do_CONNECT(self):
        if not GL_AUTHENTICATION.authenticate(self):
            ## Response already sent
            return
        if os.path.isfile(self.cakey) and os.path.isfile(self.cacert) and os.path.isfile(self.certkey) and os.path.isdir(self.certdir):
            self.connect_intercept()
        elif ProxyRequestHandler.TIME_OUT_CONNECTION:
            logger.error("Entering Timeout Infinite Loop")
            while True:
                time.sleep(1)
        elif ProxyRequestHandler.ALLOW_RELAY:
            self.connect_relay()
        else:
            logger.error("Preventing HTTPS CONNECT proxy for %s",self.path)
            self.send_error(502)


    def connect_intercept(self):
        hostname = self.path.split(':')[0]
        certpath = "%s/%s.crt" % (self.certdir.rstrip('/'), hostname)

        with self.lock:
            if not os.path.isfile(certpath):
                epoch = "%d" % (time.time() * 1000)
                p1 = Popen(["openssl", "req", "-new", "-key", self.certkey, "-subj", "/CN=%s" % hostname], stdout=PIPE)
                p2 = Popen(["openssl", "x509", "-req", "-days", "3650", "-CA", self.cacert, "-CAkey", self.cakey, "-set_serial", epoch, "-out", certpath], stdin=p1.stdout, stderr=PIPE)
                p2.communicate()

        self.wfile.write("%s %d %s\r\n" % (self.protocol_version, 200, 'Connection Established'))
        self.end_headers()

        self.connection = ssl.wrap_socket(self.connection, keyfile=self.certkey, certfile=certpath, server_side=True)
        self.rfile = self.connection.makefile("rb", self.rbufsize)
        self.wfile = self.connection.makefile("wb", self.wbufsize)

        conntype = self.headers.get('Proxy-Connection', '')
        if self.protocol_version == "HTTP/1.1" and conntype.lower() != 'close':
            self.close_connection = 0
        else:
            self.close_connection = 1

    def connect_relay(self):
        address = self.path.split(':', 1)
        address[1] = int(address[1]) or 443

        address[0] = self.dummyReplacement(address[0])

        try:
            s = socket.create_connection(address, timeout=self.timeout)
        except Exception as e:
            logger.warning("connect_relay failed with error {}".format(e))
            return self.send_error(502)
        logger.debug("connection success")

        self.send_response(200, 'Connection Established')
        self.end_headers()

        conns = [self.connection, s]
        self.close_connection = 0
        while not self.close_connection:
            rlist, wlist, xlist = select.select(conns, [], conns, self.timeout)
            if xlist or not rlist:
                break
            for r in rlist:
                other = conns[1] if r is conns[0] else conns[0]
                data = r.recv(8192)
                if not data:
                    self.close_connection = 1
                    break
                other.sendall(data)

    def do_GET(self):
        if self.path == 'http://proxy2.test/':
            return self.send_cacert()

        if not GL_AUTHENTICATION.authenticate(self):
            ## Response already sent
            return

        req = self
        content_length = int(req.headers.get('Content-Length', 0))
        req_body = self.rfile.read(content_length) if content_length else None

        if req.path[0] == '/':
            if isinstance(self.connection, ssl.SSLSocket):
                req.path = "https://%s%s" % (req.headers['Host'], req.path)
            else:
                req.path = "http://%s%s" % (req.headers['Host'], req.path)

        req_body_modified = self.request_handler(req, req_body)
        if req_body_modified is False:
            return self.send_error(403)
        elif req_body_modified is not None:
            req_body = req_body_modified
            req.headers['Content-length'] = str(len(req_body))

        u = urllib.parse.urlsplit(req.path)
        scheme, netloc, path = u.scheme, u.netloc, (u.path + '?' + u.query if u.query else u.path)
        assert scheme in ('http', 'https')

        netloc = self.dummyReplacement(netloc)

        if netloc:
            req.headers['Host'] = netloc
        setattr(req, 'headers', self.filter_headers(req.headers))

        origin = (scheme, netloc)
        for tries in range(3):
            try:
                if ProxyRequestHandler.PARENT_PROXY is None:
                    if not origin in self.tls.conns:
                        if scheme == 'https':
                            self.tls.conns[origin] = http.client.HTTPSConnection(netloc, timeout=self.timeout)
                        else:
                            self.tls.conns[origin] = http.client.HTTPConnection(netloc, timeout=self.timeout)
                elif scheme == "http":
                    logger.info("Connecting via %s to %s",ProxyRequestHandler.PARENT_PROXY.netloc,netloc)
                    if not origin in self.tls.conns:
                        path = urllib.parse.urlunsplit((scheme,netloc,path,"",""))
                        self.tls.conns[origin] = http.client.HTTPConnection(ProxyRequestHandler.PARENT_PROXY.netloc, timeout=self.timeout)
                        self.tls.conns[origin].set_tunnel(netloc)
                else:
                    return self.send_error(500)

                conn = self.tls.conns[origin]

                conn.request(self.command, path, req_body, dict(req.headers))
                res = conn.getresponse()

                version_table = {10: 'HTTP/1.0', 11: 'HTTP/1.1'}
                setattr(res, 'headers', res.msg)
                setattr(res, 'response_version', version_table[res.version])

                # support streaming
                if not 'Content-Length' in res.headers and 'no-store' in res.headers.get('Cache-Control', ''):
                    self.response_handler(req, req_body, res, '')
                    setattr(res, 'headers', self.filter_headers(res.headers))
                    self.relay_streaming(res)
                    with self.lock:
                        self.save_handler(req, req_body, res, '')
                    return

                res_body = res.read()
            except Exception as e:
                logger.warning("do_GET failed with error {}".format(e))
                if origin in self.tls.conns:
                    del self.tls.conns[origin]
                continue
            break
        else:
            return self.send_error(502)

        content_encoding = res.headers.get('Content-Encoding', 'identity')
        res_body_plain = self.decode_content_body(res_body, content_encoding)

        res_body_modified = self.response_handler(req, req_body, res, res_body_plain)
        if res_body_modified is False:
            return self.send_error(403)

        elif res_body_modified is not None:
            res_body_plain = res_body_modified
            res_body = self.encode_content_body(res_body_plain, content_encoding)
            res.headers['Content-Length'] = str(len(res_body))

        setattr(res, 'headers', self.filter_headers(res.headers))

        self.wfile.write("%s %d %s\r\n" % (self.protocol_version, res.status, res.reason))
        for line in res.headers.headers:
            self.wfile.write(line)
        self.end_headers()
        self.wfile.write(res_body)
        self.wfile.flush()

        with self.lock:
            self.save_handler(req, req_body, res, res_body_plain)

    def relay_streaming(self, res):
        self.wfile.write("%s %d %s\r\n" % (self.protocol_version, res.status, res.reason))
        for line in res.headers.headers:
            self.wfile.write(line)
        self.end_headers()
        try:
            while True:
                chunk = res.read(8192)
                if not chunk:
                    break
                self.wfile.write(chunk)
            self.wfile.flush()
        except socket.error:
            # connection closed by client
            pass

    do_HEAD = do_GET
    do_POST = do_GET
    do_PUT = do_GET
    do_DELETE = do_GET
    do_OPTIONS = do_GET

    def filter_headers(self, headers):
        # http://tools.ietf.org/html/rfc2616#section-13.5.1
        hop_by_hop = ('connection', 'keep-alive', 'proxy-authenticate', 'proxy-authorization', 'te', 'trailers', 'transfer-encoding', 'upgrade')
        for k in hop_by_hop:
            del headers[k]

        # accept only supported encodings
        if 'Accept-Encoding' in headers:
            ae = headers['Accept-Encoding']
            filtered_encodings = [x for x in re.split(r',\s*', ae) if x in ('identity', 'gzip', 'x-gzip', 'deflate')]
            headers['Accept-Encoding'] = ', '.join(filtered_encodings)

        return headers

    def encode_content_body(self, text, encoding):
        if encoding == 'identity':
            data = text
        elif encoding in ('gzip', 'x-gzip'):
            io = StringIO()
            with gzip.GzipFile(fileobj=io, mode='wb') as f:
                f.write(text)
            data = io.getvalue()
        elif encoding == 'deflate':
            data = zlib.compress(text)
        else:
            raise Exception("Unknown Content-Encoding: %s" % encoding)
        return data

    def decode_content_body(self, data, encoding):
        if encoding == 'identity':
            text = data
        elif encoding in ('gzip', 'x-gzip'):
            io = StringIO(data)
            with gzip.GzipFile(fileobj=io) as f:
                text = f.read()
        elif encoding == 'deflate':
            try:
                text = zlib.decompress(data)
            except zlib.error:
                text = zlib.decompress(data, -zlib.MAX_WBITS)
        else:
            raise Exception("Unknown Content-Encoding: %s" % encoding)
        return text

    def send_cacert(self):
        with open(self.cacert, 'rb') as f:
            data = f.read()

        self.wfile.write("%s %d %s\r\n" % (self.protocol_version, 200, 'OK'))
        self.send_header('Content-Type', 'application/x-x509-ca-cert')
        self.send_header('Content-Length', len(data))
        self.send_header('Connection', 'close')
        self.end_headers()
        self.wfile.write(data)

    def print_info(self, req, req_body, res, res_body):
        def parse_qsl(s):
            return '\n'.join("%-20s %s" % (k, v) for k, v in urllib.parse.parse_qsl(s, keep_blank_values=True))

        req_header_text = "%s %s %s\n%s" % (req.command, req.path, req.request_version, req.headers)
        res_header_text = "%s %d %s\n%s" % (res.response_version, res.status, res.reason, res.headers)

        print(with_color(33, req_header_text))

        u = urllib.parse.urlsplit(req.path)
        if u.query:
            query_text = parse_qsl(u.query)
            print(with_color(32, "==== QUERY PARAMETERS ====\n%s\n" % query_text))

        cookie = req.headers.get('Cookie', '')
        if cookie:
            cookie = parse_qsl(re.sub(r';\s*', '&', cookie))
            print(with_color(32, "==== COOKIE ====\n%s\n" % cookie))

        auth = req.headers.get('Authorization', '')
        if auth.lower().startswith('basic'):
            token = auth.split()[1].decode('base64')
            print(with_color(31, "==== BASIC AUTH ====\n%s\n" % token))

        if req_body is not None:
            req_body_text = None
            content_type = req.headers.get('Content-Type', '')

            if content_type.startswith('application/x-www-form-urlencoded'):
                req_body_text = parse_qsl(req_body)
            elif content_type.startswith('application/json'):
                try:
                    json_obj = json.loads(req_body)
                    json_str = json.dumps(json_obj, indent=2)
                    if json_str.count('\n') < 50:
                        req_body_text = json_str
                    else:
                        lines = json_str.splitlines()
                        req_body_text = "%s\n(%d lines)" % ('\n'.join(lines[:50]), len(lines))
                except ValueError:
                    req_body_text = req_body
            elif len(req_body) < 1024:
                req_body_text = req_body

            if req_body_text:
                print(with_color(32, "==== REQUEST BODY ====\n%s\n" % req_body_text))

        print(with_color(36, res_header_text))

        cookies = res.headers.getheaders('Set-Cookie')
        if cookies:
            cookies = '\n'.join(cookies)
            print(with_color(31, "==== SET-COOKIE ====\n%s\n" % cookies))

        if res_body is not None:
            res_body_text = None
            content_type = res.headers.get('Content-Type', '')

            if content_type.startswith('application/json'):
                try:
                    json_obj = json.loads(res_body)
                    json_str = json.dumps(json_obj, indent=2)
                    if json_str.count('\n') < 50:
                        res_body_text = json_str
                    else:
                        lines = json_str.splitlines()
                        res_body_text = "%s\n(%d lines)" % ('\n'.join(lines[:50]), len(lines))
                except ValueError:
                    res_body_text = res_body
            elif content_type.startswith('text/html'):
                m = re.search(r'<title[^>]*>\s*([^<]+?)\s*</title>', res_body, re.I)
                if m:
                    h = HTMLParser()
                    print(with_color(32, "==== HTML TITLE ====\n%s\n" % h.unescape(m.group(1).decode('utf-8'))))
            elif content_type.startswith('text/') and len(res_body) < 1024:
                res_body_text = res_body

            if res_body_text:
                print(with_color(32, "==== RESPONSE BODY ====\n%s\n" % res_body_text))

    def request_handler(self, req, req_body):
        pass

    def response_handler(self, req, req_body, res, res_body):
        pass

    def save_handler(self, req, req_body, res, res_body):
        self.print_info(req, req_body, res, res_body)

def createServer(port, HandlerClass, ServerClass):
    ServerClass.address_family = socket.AF_INET

    for server in ('', 'localhost', '127.0.0.1'):
        server_address = (server, port)
        try:
            return ServerClass(server_address, HandlerClass)
        except socket.error as e:
            logger.warning("Failed to serve to {} via IPv4 with error {}".format(server, e))

    ServerClass.address_family = socket.AF_INET6

    for server in ('', 'localhost', '127.0.0.1'):
        server_address = (server, port)
        try:
            return ServerClass(server_address, HandlerClass)
        except socket.error as e:
            logger.warning("Failed to serve to {} via IPv6with error {}".format(server,e))

    return None


def test(HandlerClass=ProxyRequestHandler, ServerClass=ThreadingHTTPServer, protocol="HTTP/1.1"):
    ppid = os.getppid()
    logging.basicConfig(format='%(asctime)s %(levelname)-8s %(name)-15s %(message)s')
    logging.getLogger().setLevel(logging.DEBUG)

    parser = optparse.OptionParser()
    parser.add_option("--no-relay",dest="relay",default=True,action="store_false",
        help="Don't do SSL relay CONNECT")
    parser.add_option("--timeout-connections",dest="timeout",default=False,action="store_true",
        help="Any connection will timeout in infinite loop")
    parser.add_option("--proxy","--parent",dest="parent_proxy",default=None,
        help="Parent Proxy")
    parser.add_option("--username",dest="username",default=None,
        help="Username if authentication required")
    parser.add_option("--password",dest="password",default=None,
        help="Password if authentication required")
    parser.add_option("--basic","--auth",dest="basic",default=False,action="store_true",
        help="Require BASIC proxy authentication")
    parser.add_option("--digest",dest="digest",default=False,action="store_true",
        help="Require DIGEST proxy authentication")
    parser.add_option("--certfile",dest="certfile", default=None, action="store",
        help="Certificate file for https connections")
    parser.add_option("--tls1_2",dest="tls1_2", default=False, action="store_true",
        help="Run with tls 1.2")
    parser.add_option("--tls1_1",dest="tls1_1", default=False, action="store_true",
        help="Run with tls 1.1")
    parser.add_option("--tls1",dest="tls1", default=False, action="store_true",
        help="Run with tls 1.0")

    (options, args) = parser.parse_args()

    if len(args) > 0:
        port = int(args[0])
    else:
        port = 8080

    ProxyRequestHandler.OPTIONS = options
    ProxyRequestHandler.ALLOW_RELAY = options.relay
    logger.info("Timeout Value {}".format(options.timeout))
    ProxyRequestHandler.TIME_OUT_CONNECTION = options.timeout
    if options.parent_proxy:
        ProxyRequestHandler.PARENT_PROXY = urllib.parse.urlsplit(options.parent_proxy)

    passwordManager = https_proxy_lib.HTTPAuthentication.PasswordManager(
        options.username,
        options.password)

    global GL_AUTHENTICATION
    if options.basic:
        GL_AUTHENTICATION = https_proxy_lib.HTTPAuthentication.BasicAuthentication(passwordManager)
    elif options.digest:
        GL_AUTHENTICATION = https_proxy_lib.HTTPAuthentication.DigestAuthentication(passwordManager)

    HandlerClass.protocol_version = protocol

    httpd = createServer(port, HandlerClass, ServerClass)
    if httpd is None:
        logger.fatal("Failed to serve!")
        return 1
    
    protocol = None
    if options.tls1:
        protocol=ssl.PROTOCOL_TLSv1
    if options.tls1_1:
        protocol=ssl.PROTOCOL_TLSv1_1
    if options.tls1_2:
        protocol=ssl.PROTOCOL_TLSv1_2
        
    if protocol:
        if not options.certfile:
            raise AssertionError("Certfile needs to be set if using tls options")
        httpd.socket = ssl.wrap_socket(
                httpd.socket,
                server_side=True,
                ssl_version=protocol,
                certfile=options.certfile)
    
    sa = httpd.socket.getsockname()
    logger.info("Serving HTTP Proxy on %s port %d...", sa[0], sa[1] )
    while ppid == os.getppid():
        httpd.handle_request()

    return 0

if __name__ == '__main__':
    sys.exit(test())

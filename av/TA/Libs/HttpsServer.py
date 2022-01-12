#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2020 Sophos Plc, Oxford, England.
# All rights reserved.


import ssl
import subprocess
import sys
import threading

import http.server

from ArgParserUtils import tls_from_string

from robot.api import logger


class HttpsHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, request, client_address, server):
        buffer = 1
        self.log_file = open('/tmp/https_server.log', 'w', buffer)
        http.server.SimpleHTTPRequestHandler.__init__(self, request, client_address, server)

    def log_message(self, format, *args):
        self.log_file.write("%s - - [%s] %s\n" %
                            (self.client_address[0],
                             self.log_date_time_string(),
                             format % args))

    def handle_request(self, verb):
        self.log_message("%s", "Received HTTP {} Request".format(verb))
        self.log_message("%s", "Headers:\n{}".format(self.headers))
        content_length = int(self.headers["Content-Length"])
        content = self.rfile.read(content_length)
        self.log_message("%s", "Content: {}".format(content))
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def handle_get_request(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(b"<html><head><title>Response</title></head><body>Response From HttpsServer</body></html>")

    def do_POST(self):
        self.handle_request("POST")

    def do_PUT(self):
        self.handle_request("PUT")

    def do_GET(self):
        self.handle_get_request()


class SophosHTTPServer(http.server.HTTPServer):
    def handle_timeout(self):
        logger.error("Timeout waiting for HTTPS request")


OPENSSL="/usr/bin/openssl"


class HttpsServer(object):
    def __init__(self):
        self.thread = None
        self.m_last_port = None
        self.m_keyfile_path = None
        self.m_server = None

    def __generate_key(self, certfile_path):
        if self.m_keyfile_path is None:
            keyfile_path = "/tmp/key.pem"
            subprocess.check_call([
                OPENSSL,
                'genrsa',
                '-out', keyfile_path,
                '4096'])
            self.m_keyfile_path = keyfile_path

        subject = "/C=GB/ST=London/L=London/O=Sophos/OU=ESG/CN=localhost"
        subprocess.check_call('/usr/bin/openssl req -x509 -key {} -out {} -days 2 -nodes -subj "{}"'
                              .format(self.m_keyfile_path, certfile_path, subject), shell=True)

        return self.m_keyfile_path

    def start_https_server(self, certfile_path, port=443, protocol_string=None):
        """
        Starts a one-shot HTTPS server - only serves one request
        :param certfile_path:
        :param port:
        :param protocol_string:
        :return:
        """
        if self.thread is not None:
            self.stop_https_server()

        port = int(port)
        print("Start Simple HTTPS Server localhost:{}".format(port))
        self.m_last_port = port

        httpd = SophosHTTPServer(('', port), HttpsHandler)
        httpd.timeout = 60  # stop server after timeout

        protocol = tls_from_string(protocol_string)
        if not protocol:
            protocol = ssl.PROTOCOL_TLS

        keyfile_path = self.__generate_key(certfile_path)

        httpd.socket = ssl.wrap_socket(httpd.socket,
                                       server_side=True,
                                       keyfile=keyfile_path,
                                       ssl_version=protocol,
                                       certfile=certfile_path)

        self.thread = threading.Thread(target=httpd.handle_request)
        self.thread.daemon = True
        self.thread.start()
        self.m_server = httpd

    def stop_https_server(self):
        if self.m_server:
            self.m_server.server_close()
            self.m_server = None

        if self.thread:
            self.thread.join()
            self.thread = None
            self.m_last_port = None

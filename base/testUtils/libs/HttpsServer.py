#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2020 Sophos Plc, Oxford, England.
# All rights reserved.


import ssl
import subprocess
import threading

import http.server

from ArgParserUtils import tls_from_string


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

    def do_POST(self):
        self.handle_request("POST")

    def do_PUT(self):
        self.handle_request("PUT")


class HttpsServer(object):
    def __init__(self):
        self.thread = None

    def start_https_server(self, certfile_path, port=443, protocol_string=None):

        port = int(port)
        print("Start Simple HTTPS Server localhost:{}".format(port))

        keyfile_path = "/tmp/key.pem"
        subject = "/C=GB/ST=London/L=London/O=Sophos/OU=ESG/CN=localhost"
        subprocess.call('/usr/bin/openssl req -x509 -newkey rsa:4096 -keyout {} -out {} -days 365 -nodes -subj "{}"'
                        .format(keyfile_path, certfile_path, subject), shell=True)

        httpd = http.server.HTTPServer(('', port), HttpsHandler)

        protocol = tls_from_string(protocol_string)
        if not protocol:
            protocol = ssl.PROTOCOL_TLS

        httpd.socket = ssl.wrap_socket(httpd.socket,
                                       server_side=True,
                                       keyfile=keyfile_path,
                                       ssl_version=protocol,
                                       certfile=certfile_path)

        self.thread = threading.Thread(target=httpd.handle_request)
        self.thread.daemon = True
        self.thread.start()

    def stop_https_server(self):
        if self.thread:
            self.thread.join()

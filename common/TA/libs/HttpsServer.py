#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2020 Sophos Plc, Oxford, England.
# All rights reserved.


import ssl
import subprocess
import threading
import time
import os

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
        self.log_message("url path: " + self.path)
        if self.path == "/upload":
            with open("/tmp/upload.zip", 'wb') as f:
                f.write(content)

        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def handle_get_request(self):

        textfile = os.path.isfile("/tmp/downloadToUnzip.txt")
        if textfile:
            targetpath = "/tmp/downloadToUnzip.txt"
        else:
            targetpath = "/tmp/testdownload.zip"

        if self.path == "/download":
            self.log_message("Download action received")
            with open(targetpath, "rb") as file:
                contents = file.read()

            self.log_message("%s", contents)
            self.protocol_version = 'HTTP/1.1'
            self.send_response(200)

            if textfile:
                self.send_header('Content-type', "text/plain")
            else:
                self.send_header('Content-type', "application/zip")

            self.send_header('Content-length', len(contents))
            self.send_header("Content-Disposition", f"attachment; filename=/opt/sophos-spl/plugins/responseactions{targetpath}")
            self.end_headers()
            self.wfile.write(contents)

            os.remove(targetpath)
        else:
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()

            self.wfile.write(b"<html><head><title>Response</title></head><body>Response From HttpsServer</body></html>")

    def handle_put_request_with_hang(self):
        self.log_message("Sleeping for 30s")
        time.sleep(30)
        self.log_message("Done sleeping")

    def do_POST(self):
        self.handle_request("POST")

    def do_PUT(self):
        if os.environ.get("BREAK_PUT_REQUEST"):
            self.handle_put_request_with_hang()
        else:
            self.handle_request("PUT")

    def do_GET(self):
        self.log_message("Received HTTP GET Request")
        self.handle_get_request()


class HttpsServer(object):
    def __init__(self):
        self.thread = None
        self.httpd = None

    def start_https_server(self, certfile_path, port=443, protocol_string=None, root_ca=False):

        port = int(port)
        print("Start Simple HTTPS Server localhost:{}".format(port))

        keyfile_path = "/tmp/key.pem"
        subject = "/C=GB/ST=London/L=London/O=Sophos/OU=ESG/CN=localhost"

        if root_ca:
            keyfile_path = "localhost.key"
            #generate private key
            subprocess.call('/usr/bin/openssl genrsa -out localhost.key 2048', shell=True)

            #generate root cert
            subprocess.call('/usr/bin/openssl req -new -x509 -key localhost.key -subj {} -out /tmp/ca.crt'.format(subject), shell=True)

            #generate csr info
            subprocess.call('/usr/bin/openssl req -new -sha256 -key localhost.key -nodes -subj {} -out localhost.csr'
                            .format(subject), shell=True)

            #generate child cert
            subprocess.call('/usr/bin/openssl x509 -req -in localhost.csr -CA /tmp/ca.crt -CAkey localhost.key -CAcreateserial -out {}'.format(certfile_path), shell=True)

        else:
            subprocess.call('/usr/bin/openssl req -x509 -newkey rsa:4096 -keyout {} -out {} -days 365 -nodes -subj "{}"'
                            .format(keyfile_path, certfile_path, subject), shell=True)

        self.httpd = http.server.HTTPServer(('', port), HttpsHandler)

        protocol = tls_from_string(protocol_string)
        if not protocol:
            protocol = ssl.PROTOCOL_TLS

        self.httpd.socket = ssl.wrap_socket(self.httpd.socket,
                                       server_side=True,
                                       keyfile=keyfile_path,
                                       ssl_version=protocol,
                                       certfile=certfile_path)

        self.thread = threading.Thread(target=self.httpd.serve_forever)
        self.thread.daemon = True
        self.thread.start()

    def stop_https_server(self):
        if self.httpd:
            self.httpd.server_close()
            self.httpd.shutdown()
        if self.thread:
            self.thread.join()

#!/usr/bin/python
# -*- coding: utf-8 -*-
# Derived from Perforce //dev/savlinux/regression/supportFiles/401server.py



import os
import sys
import http.server
import ssl

import cloudServer

class LoggingSSLSocket(ssl.SSLSocket):

    def accept(self, *args, **kwargs):
        result = super(LoggingSSLSocket, self).accept(*args, **kwargs)
        return result

    def do_handshake(self, *args, **kwargs):
        result = super(LoggingSSLSocket, self).do_handshake(*args, **kwargs)
        return result

class Handler(http.server.BaseHTTPRequestHandler):
    """ Main class to present webpages and authentication. """
    def do_HEAD(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_AUTHHEAD(self):
        self.send_response(401)
        self.send_header('WWW-Authenticate', 'Basic realm=\"Test\"')
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_GET(self):
        self.send_response(401)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        self.wfile.write("<html><head><title>No webpage for you.</title></head>")
        self.wfile.write("<body><p>No webpage for you.</p>")
        return False


def main():
    certfile = cloudServer.getServerCert()

    if not os.path.exists(certfile):
        print("Cannot find certfile: %s"%certfile, file=sys.stderr)
        return 1

    httpd = http.server.HTTPServer(('localhost', 4443), Handler)
    httpd.socket = ssl.wrap_socket(httpd.socket, certfile=certfile, server_side=True)
    httpd.serve_forever()

    return 0


if __name__ == '__main__':
    sys.exit(main())

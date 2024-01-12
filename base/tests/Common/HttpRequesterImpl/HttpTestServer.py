#!/usr/bin/env python3

# This script runs multiple test servers that our http client lib can use to test against.
# Currently, this script runs in parallel: HTTP, HTTPS, proxy, basic auth proxy
# URLS are used to distinguish test cases, e.g. you might have a URL such as "localhost/getWithHeadersTest", using this
# convention we can write test cases here and target them from test code, knowing what the result should be.

import os
import ssl
import subprocess
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer
import logging
import logging.handlers
import urllib.parse
import socket

import psutil
import time
import argparse
import sys
import signal

class TestServer(BaseHTTPRequestHandler):
    def getUrlParts(self, path):
        return urllib.parse.urlparse(self.path)

    def _set_response(self, status: int, headers: dict):
        self.send_response(status)
        # self.send_header('Content-type', 'text/html')
        for header, value in headers.items():
            self.send_header(header, value)
        self.end_headers()

    def do_GET(self):
        url_parts = self.getUrlParts(self.path)
        resource_path = url_parts.path
        query_str = url_parts.query
        request_headers = self.headers

        response_body = ""
        response_headers = {}
        response_code = 200
        logging.info(f"Running test: '{resource_path}'")

        if resource_path == "/getWithPort" or resource_path == "/httpsGetWithPort":
            response_body = f"{resource_path} response body"
            response_code = 200
            response_headers = {"test_header": "test_header_value"}
        elif resource_path == "/getWithPortAndHeaders":
            if "req_header" in request_headers and request_headers["req_header"] == "a value":
                response_code = 200
                response_body = f"{resource_path} response body"
            else:
                response_body = "Test Failed - headers not correct"
                response_code = 400
        elif resource_path == "/getWithPortAndParameters":
            if query_str == "param1=value1":
                response_code = 200
                response_body = f"{resource_path} response body"
            else:
                response_body = "Test Failed - parameters not correct"
                response_code = 400
        elif resource_path == "/getWithFileDownloadAndExplicitFileName":
            response_body = "Sample text file to use in HTTP tests."
            response_code = 200
        elif resource_path == "/getWithFileDownloadToDirectory/sample.txt":
            response_body = "Sample text file to use in HTTP tests."
            response_code = 200
        elif resource_path == "/getWithFileDownloadToDirectoryAndContentDispositionHeader/sample.txt":
            response_body = "Sample text file to use in HTTP tests."
            response_headers = {"Content-Disposition": 'attachment; filename="differentName.txt"; useless="blah"'}
            response_code = 200
        elif resource_path == "/getWithPortAndTimeout":
            # Test uses a timeout of 3, so this needs to be > 3
            time.sleep(5)
            return
        elif resource_path == "/getWithPortAndBandwidthLimit":
            # respond with 1000 bytes
            response_body = "a" * 1000
            response_code = 200
        else:
            response_body = "Not a valid test case!"
            logging.warning(f"Not a valid test case: GET - '{resource_path}'")
            response_code = 404
            # print("not a valid test case name")

        response_headers["Content-Length"] = len(response_body.encode('utf-8'))
        self._set_response(response_code, response_headers)
        self.wfile.write(response_body.encode('utf-8'))

    def do_POST(self):
        url_parts = self.getUrlParts(self.path)
        resource_path = url_parts.path
        query_str = url_parts.query
        request_headers = self.headers

        response_body = ""
        response_headers = {}
        response_code = 200

        logging.info(f"Running test: '{resource_path}'")

        content_length = 0
        if "Content-Length" in self.headers:
            content_length = int(self.headers['Content-Length'])
        put_data = self.rfile.read(content_length)

        if resource_path == "/postWithPort":
            response_body = f"{resource_path} response body"
            response_code = 200
            response_headers = {"test_header": "test_header_value"}
        elif resource_path == "/postWithData":
            if put_data.decode('utf-8') == "SamplePostData":
                response_body = f"{resource_path} response body"
                response_code = 200
            else:
                response_body = f"{resource_path} test failed, posted data id not match test case"
                response_code = 400
        elif resource_path == "/postWithFileUpload":
            response_body = f"{resource_path} response body, you PUT {content_length} bytes"
            response_code = 200
            response_headers = {"test_header": "test_header_value"}

        else:
            response_body = "Not a valid test case!"
            logging.warning(f"Not a valid test case: PUT - '{resource_path}'")
            response_code = 404
        response_headers["Content-Length"] = len(response_body.encode('utf-8'))
        self._set_response(response_code, response_headers)
        self.wfile.write(response_body.encode('utf-8'))

    def do_PUT(self):
        url_parts = self.getUrlParts(self.path)
        resource_path = url_parts.path
        query_str = url_parts.query
        request_headers = self.headers

        response_body = ""
        response_headers = {}
        response_code = 200

        logging.info(f"Running test: '{resource_path}'")

        content_length = 0
        if "Content-Length" in self.headers:
            content_length = int(self.headers['Content-Length'])
        put_data = self.rfile.read(content_length)

        if resource_path == "/putWithPort":
            response_body = f"{resource_path} response body"
            response_code = 200
            response_headers = {"test_header": "test_header_value"}
        elif resource_path == "/putWithFileUpload":
            response_body = f"{resource_path} response body, you PUT {content_length} bytes"
            response_code = 200
            response_headers = {"test_header": "test_header_value"}
        else:
            response_body = "Not a valid test case!"
            logging.warning(f"Not a valid test case: PUT - '{resource_path}'")
            response_code = 404

        response_headers["Content-Length"] = len(response_body.encode('utf-8'))
        self._set_response(response_code, response_headers)
        self.wfile.write(response_body.encode('utf-8'))

    def do_DELETE(self):
        url_parts = self.getUrlParts(self.path)
        resource_path = url_parts.path
        query_str = url_parts.query
        request_headers = self.headers

        response_body = ""
        response_headers = {}
        response_code = 200

        logging.info(f"Running test: '{resource_path}'")

        content_length = 0
        if "Content-Length" in self.headers:
            content_length = int(self.headers['Content-Length'])
        data = self.rfile.read(content_length)

        if resource_path == "/deleteWithPort":
            response_body = f"{resource_path} response body"
            response_code = 200
            response_headers = {"test_header": "test_header_value"}
        else:
            response_body = "Not a valid test case!"
            logging.warning(f"Not a valid test case: DELETE - '{resource_path}'")
            response_code = 404

        response_headers["Content-Length"] = len(response_body.encode('utf-8'))
        self._set_response(response_code, response_headers)
        self.wfile.write(response_body.encode('utf-8'))

    def do_OPTIONS(self):
        url_parts = self.getUrlParts(self.path)
        resource_path = url_parts.path
        query_str = url_parts.query
        request_headers = self.headers

        response_body = ""
        response_headers = {}
        response_code = 200

        logging.info(f"Running test: '{resource_path}'")

        content_length = 0
        if "Content-Length" in self.headers:
            content_length = int(self.headers['Content-Length'])
        data = self.rfile.read(content_length)

        if resource_path == "/optionsWithPort":
            response_body = f"{resource_path} response body"
            response_code = 200
            response_headers = {"test_header": "test_header_value"}
        else:
            response_body = "Not a valid test case!"
            logging.warning(f"Not a valid test case: OPTIONS - '{resource_path}'")
            response_code = 404

        response_headers["Content-Length"] = len(response_body.encode('utf-8'))
        self._set_response(response_code, response_headers)
        self.wfile.write(response_body.encode('utf-8'))

def create_server(port: int, https: bool, server_class=HTTPServer, handler_class=TestServer):
    logging.basicConfig(level=logging.INFO)
    server_address = ('localhost', port)
    url_protocol = "http"
    httpd = server_class(server_address, handler_class)
    if https:
        url_protocol = "https"
        this_dir = os.path.dirname(os.path.abspath(__file__))
        cert_filename = "localhost-selfsigned.crt"
        key_filename = "localhost-selfsigned.key"
        cert_filepath = os.path.join(this_dir, cert_filename)
        key_filepath = os.path.join(this_dir, key_filename)
        if not os.path.exists(key_filepath) or not os.path.exists(cert_filepath):
            # If a key needs to be changed or regenerated, this is the command used to generate a key and self-signed cert
            # openssl req -x509 -nodes -days 3650 -newkey rsa:2048 -keyout localhost-selfsigned.key -out localhost-selfsigned.crt -subj "/CN=localhost"
            # -subj "/C=GB/ST=London/L=London/O=Global Security/OU=IT Department/CN=example.com"
            ret_code = subprocess.call(f'openssl req -x509 -nodes -days 3650 -newkey rsa:2048 -keyout {key_filepath} -out {cert_filepath}  -subj "/CN=localhost"', shell=True)
            if ret_code != 0:
                logging.error("Failed to generate certs.")
                exit(1)
        httpd.socket = ssl.wrap_socket(httpd.socket, server_side=True, keyfile=key_filepath, ssl_version=ssl.PROTOCOL_TLSv1_2, certfile=cert_filepath)
        logging.info(f"Creating HTTPS server on port: {port} with cert: {cert_filepath}")
    else:
        logging.info(f"Creating HTTP server on port: {port}")
    return httpd

def is_proxy_up(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect(("localhost", int(port)))
        s.close()
        return True
    except EnvironmentError:
        return False

def wait_for_proxy_to_be_up(port):
    tries = 100
    while tries > 0:
        if is_proxy_up(port):
            return True
        tries -= 1
        time.sleep(1)
    return AssertionError(f"Proxy hasn't started up after {tries} seconds port={port}")


def setup_logger():
    this_dir = os.path.dirname(os.path.abspath(__file__))
    log_location = os.path.join(this_dir, "http_test_server.log")
    rootLogger = logging.getLogger()
    rootLogger.setLevel(logging.DEBUG)

    streamHandler = logging.StreamHandler()
    streamHandler.setLevel(logging.DEBUG)

    rootLogger.addHandler(streamHandler)

    rotatingFileHandler = logging.handlers.RotatingFileHandler(log_location, maxBytes=1024*1024, backupCount=2)
    rotatingFileHandler.setLevel(logging.DEBUG)

    rootLogger.addHandler(rotatingFileHandler)
    print("Logging to: {log_location}")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="This server runs test cases to help verify HTTP clients. It can be run in HTTP or HTTPS mode.")
    parser.add_argument('--http-port', type=int, help='Port number to run HTTP server on.', default=7780)
    parser.add_argument('--https-port', type=int, help='Port number to run HTTPS server on.', default=7743)
    parser.add_argument('--proxy-port', type=int, help='Port number to run proxy server on.', default=7750)
    parser.add_argument('--proxy-port-basic-auth', type=int, help='Port number to run basic auth proxy server on.', default=7751)
    args = parser.parse_args()
    setup_logger()
    running = True

    http_server = create_server(port=args.http_port, https=False)
    https_server = create_server(port=args.https_port, https=True)

    http_thread = threading.Thread(target=http_server.serve_forever)
    http_thread.start()
    logging.info(f"HTTP server started http://localhost:{args.http_port}")

    https_thread = threading.Thread(target=https_server.serve_forever)
    https_thread.start()
    logging.info(f"HTTPS server started https://localhost:{args.https_port}")

    logging.info(f"Starting unauthenticated proxy server on port: {args.proxy_port}")
    no_auth_proxy_cmd = ["proxy", "--num-workers", "1", "--port", str(args.proxy_port), "--hostname", "127.0.0.1"]
    no_auth_proxy_processes = subprocess.Popen(no_auth_proxy_cmd)

    logging.info(f"Starting basic auth proxy server on port: {args.proxy_port_basic_auth}")
    basic_auth_proxy_cmd = ["proxy", "--num-workers", "1", "--port", str(args.proxy_port_basic_auth), "--hostname", "127.0.0.1", "--basic-auth", "user:password"]
    basic_auth_proxy_processes = subprocess.Popen(basic_auth_proxy_cmd)

    def signal_handler(signal, frame):
        global running
        logging.info("Stopping")
        running = False

    children = []
    signal.signal(signal.SIGINT, signal_handler)
    while running:
        time.sleep(1)
        pid = os.getpid()
        this_proc = psutil.Process(pid)
        for child in this_proc.children(recursive=True):
            if child.pid not in children:
                children.append(child.pid)
                print(f"print CURRENT CHILDREN: {children}")
                logging.info(f"print CURRENT CHILDREN: {children}")

    logging.info("Shutting down no auth proxy server")
    no_auth_proxy_processes.kill()

    logging.info("Shutting down basic auth proxy server")
    basic_auth_proxy_processes.kill()

    logging.info("Shutting down HTTP server")
    http_server.server_close()
    http_server.shutdown()

    logging.info("Shutting down HTTPS server")
    https_server.server_close()
    https_server.shutdown()

    if http_thread:
        http_thread.join()

    if https_thread:
        https_thread.join()

    for child in children:
        os.kill(child, signal.SIGINT)

    logging.info("All servers stopped.")

#!/usr/bin/env python3
# Usage: HttpTestServer.py <port>

from http.server import BaseHTTPRequestHandler, HTTPServer
import logging
import urllib.parse
import time

class S(BaseHTTPRequestHandler):
    def getUrlParts(self, path):
        return urllib.parse.urlparse(self.path)
        # query_str_from_url = url_parts.query
        # resource_path = url_parts.path

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

        if resource_path == "/getWithPort":
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
            response_body = f"{resource_path} response body"
            response_code = 200
            # Test uses a timeout of 3, so this needs to be > 3
            time.sleep(5)

        else:
            response_body = "NOT A VALID TEST CASE!"
            logging.warning(f"Not a vlaid test case: GET - '{resource_path}'")
            response_code = 404
            # print("not a valid test case name")

        self._set_response(response_code, response_headers)

        # logging.info("GET request,\nPath: %s\nHeaders:\n%s\nParams:%s\n", str(self.path), str(self.headers), str(query_str_from_url))

        # self.wfile.write(f"GET request for {self.path}, with params: {str(query_str_from_url)}".encode('utf-8'))
        self.wfile.write(response_body.encode('utf-8'))

    def do_POST(self):
        content_length = int(self.headers['Content-Length']) # <--- Gets the size of data
        post_data = self.rfile.read(content_length) # <--- Gets the data itself
        logging.info("POST request,\nPath: %s\nHeaders:\n%s\n\nBody:\n%s\n",
                     str(self.path), str(self.headers), post_data.decode('utf-8'))

        self._set_response()
        self.wfile.write("POST request for {}".format(self.path).encode('utf-8'))

def run(server_class=HTTPServer, handler_class=S, port=7780):
    logging.basicConfig(level=logging.INFO)
    server_address = ('', port)
    print("Server started http://%s:%s" % ("localhost", port))
    httpd = server_class(server_address, handler_class)
    logging.info('Starting httpd...\n')
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
    logging.info('Stopping httpd...\n')


if __name__ == '__main__':
    from sys import argv

    if len(argv) == 2:
        run(port=int(argv[1]))
    else:
        run()

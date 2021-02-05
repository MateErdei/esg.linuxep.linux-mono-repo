#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2020 Sophos Plc, Oxford, England.
# All rights reserved.
"""Mock DarkBytes Server"""

import datetime
import json
import logging
import os
import io
import gzip
from pathlib import Path
from functools import partial
from threading import Thread
import ssl
import sys
import shutil
import base64
import random
from http.server import BaseHTTPRequestHandler, HTTPServer

logger = logging.getLogger('MockDarkBytes')

CURRENT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
DARK_BYTES_SERVER_DIR = r'/tmp/test/mock_darkbytes'

EMPTY_DISTRIBUTED = {
    "queries": {
    },
    "node_invalid": False
}

FAILED_ENROLL_RESPONSE = {"node_invalid": True}
NO_ERROR_RESPONSE = {"node_invalid": False}

ENROLL_RESPONSE = {
    "node_key": "",  # Optionally blank
    "node_invalid": False  # Optional, return true to indicate failure.
}

ENROLL_RESET = {
    "count": 1,
    "max": 3,
}

ACCEPT_ANY_ENROLMENT = "IGNOREME"

def decompress(compressed_bytes):
    iobytes = io.BytesIO(compressed_bytes)
    with gzip.open(iobytes) as gz:
      return gz.read()


class RealSimpleHandler(BaseHTTPRequestHandler):
    """HTTP handler for the mock server"""
    def __init__(self, server_root_dir, enrollment_secret, *args, **kwargs):
        self.server_root_dir = server_root_dir        
        self.statuses = os.path.join(self.server_root_dir, 'statuses')
        self.results = os.path.join(self.server_root_dir, 'results')
        self.distributed = os.path.join(self.server_root_dir, 'distributed')
        self.distributed_results = os.path.join(self.distributed, 'results')
        self.config_file = os.path.join(self.server_root_dir, 'config.json')
        self.pending_request = os.path.join(self.server_root_dir, 'newrequest.txt')
        self.pending_raw_request = os.path.join(self.server_root_dir, 'newrawrequest.txt')
        self.enrollment_secret = enrollment_secret
        self.create_folders()
        super(RealSimpleHandler, self).__init__(*args, **kwargs)

    def create_folders(self):
        """Create folders to store statuses and results locally"""
        os.makedirs(self.statuses, exist_ok=True)
        os.makedirs(self.results, exist_ok=True)
        os.makedirs(self.distributed, exist_ok=True)
        os.makedirs(self.distributed_results, exist_ok=True)

    def do_GET(self):  # pylint: disable=invalid-name
        """Do get"""
        logger.debug("RealSimpleHandler::get %s", self.path)
        if self.path == '/config':
            self.config(self.request)
        else:
            self._reply({"default":"true"})

    def do_HEAD(self):  # pylint: disable=invalid-name
        """Do head"""
        logger.debug("RealSimpleHandler::head %s", self.path)
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()

    
    def _extract_body(self):
        content_len = int(self.headers.get_all('Content-Length')[0])
        body = self.rfile.read(content_len)
        logger.debug("Post path: {}".format(self.path))
        logger.debug('Headers: {}'.format(self.headers))
        encoding = self.headers.get_all('Content-Encoding')
        if encoding and encoding[0] == 'gzip':
            body = decompress(body)
        logger.debug("Post body: {}".format(body))
        try:
            request = json.loads(body)
            return request
        except:
            return body

    def _get_file_content(self, file_path):
        with open(file_path,'r') as file_handler:
            content = file_handler.read()
            return content

    def _save_results(self, directory, content_as_dict):
        time = datetime.datetime.now().isoformat().replace(':', '-')
        os.makedirs(directory, exist_ok=True)
        file_path = os.path.join(directory, time + '.json')
        logger.info('Saving result to: %s', file_path)
        with open(file_path, 'w') as file:
            file.write(json.dumps(content_as_dict))

    def do_POST(self):  # pylint: disable=invalid-name
        """Do post"""
        request = self._extract_body()

        path_dispatcher = {
            '/api/enroll': self.enroll,
            '/api/config': self.config,
            '/api/log': self.process_log_data,
            '/api/distributed/read': self.distributed_read,
            '/api/distributed/write': self.distributed_write,
            '/respond/get': self.get_pending_action,
            '/respond/result': self.respond_provided,
            '/': self.process_log_data,            
        }
        
        if path_dispatcher.get(self.path, None):
            path_dispatcher[self.path](request)
        else:
            logger.warning("Unknown post request path: {}, content: {}".format(self.path, self.request))
            self._reply(NO_ERROR_RESPONSE)
        logger.debug("Post served\n\n")
               
    def get_pending_action(self, request):
        logger.info("RealSimpleHandler::respond::get")
        logger.debug("Request: {}".format(request))
        if os.path.exists(self.pending_request):
            content = self._get_file_content(self.pending_request)
            os.remove(self.pending_request)
            logger.info("Request new command: {}".format(content))
            cmd = base64.b64encode(content.encode('utf-8'))
            cmd = cmd.decode('utf-8')
            cmd_id = str(random.randint(1000, 9000))
            self._reply({"actions":[{"id":cmd_id,"command":cmd, "actionName":"test", "documentId":"nothing","params":{"sessionId":"any"}}]})
        elif os.path.exists(self.pending_raw_request):
            content = self._get_file_content(self.pending_raw_request)
            action = json.loads(content)
            logger.info("Request raw command: {}".format(content))
            os.remove(self.pending_raw_request)
            self._reply({"actions":[action]})
        else:
            self._reply({})

    def respond_provided(self, request):
        # how responses to the actions will be route. add this information to its own path. 
        logger.info("RealSimpleHandler::respond::result")
        logger.debug("Request: {}".format(request))
        decoded = base64.b64decode(request['result']).decode('utf-8')
        actions = os.path.join(self.results, 'actions')
        self._save_results(actions, {'id':request['id'], 'value':decoded})
        self._reply({})
        

    def enroll(self, request):        
        """A basic enrollment endpoint"""

        # This endpoint expects an "enroll_secret" POST body variable.
        # Over TLS, this string may be a shared secret value installed on every
        # managed host in an enterprise.

        # Alternatively, each client could authenticate with a TLS client cert.
        # Then, access to the enrollment endpoint implies the required auth.
        # A generated node_key is still supplied for identification.
        logger.info("RealSimpleHandler::enroll {}".format(request))
        if self.enrollment_secret != request["enroll_secret"]:
            if self.enrollment_secret != ACCEPT_ANY_ENROLMENT:
                logger.info("Enrollment failed values: {}, {}".format(self.enrollment_secret, request["enroll_secret"]  ))
                self._reply(FAILED_ENROLL_RESPONSE)
                return
        self._reply(ENROLL_RESPONSE)

    def config(self, request):
        """A basic config endpoint"""

        # This endpoint responds with a JSON body that is the entire config
        # content. There is no special key or status.

        # Authorization is simple authentication (the ability to download the
        # config data) using a "valid" node_key. Validity means the node_key is
        # known to this server. This toy server delivers a shared node_key,
        # imagine generating a unique node_key per enroll request, tracking the
        # generated keys, and asserting a match.

        # The osquery TLS config plugin calls the TLS enroll plugin to retrieve
        # a node_key, then submits that key alongside config/logger requests.
        logger.info("RealSimpleHandler::config {}".format(request))
        if "node_key" not in request:
            self._reply(FAILED_ENROLL_RESPONSE)
            return

        # This endpoint will also invalidate the node secret key (node_key)
        # after several attempts to test re-enrollment.
        ENROLL_RESET["count"] += 1
        if ENROLL_RESET["count"] % ENROLL_RESET["max"] == 0:
            ENROLL_RESET["first"] = 0
            self._reply(FAILED_ENROLL_RESPONSE)
            return

        with open(self.config_file, 'r') as config:
            json_string = config.read()
            self._reply(json.loads(json_string))

    def distributed_read(self, request):
        """A basic distributed read endpoint"""
        logger.info("RealSimpleHandler::distribute_read")
        logger.debug("Request: {}".format(request))
        if "node_key" not in request:
            logger.info("Reply failed to enroll")
            self._reply(FAILED_ENROLL_RESPONSE)
            return
        issue_live_query_path = os.path.join(self.distributed, 'live_query.txt')
        if issue_live_query_path:
            query = self._get_file_content(issue_live_query_path)
            os.remove(issue_live_query_path)
            test_query = {"queries": {"id1": query, "id2": "SELECT * FROM osquery_schedule;"}, "node_invalid": False}
            self._reply(test_query)
        else:
            self._reply(EMPTY_DISTRIBUTED)
        
    def distributed_write(self, request):        
        """A basic distributed write endpoint"""
        logger.info("RealSimpleHandler::distribute_write {}".format(request))
        self._save_results(self.distributed_results, request)
        self._reply({"distr":"write"})

    def process_log_data(self, request):
        logger.info("RealSimpleHandler::log_data")
        logger.debug("Request: {}".format(request))
        """Process the log data sent from osquery"""
        log_type = request['log_type']
        if log_type == 'status':
            self._process_status(request)
        if log_type == 'result':
            self._process_query_result(request)
        self._reply({"log":"suc"})

    def _process_status(self, request):
        """Process status"""
        logger.info("Received status")
        self._save_results(self.statuses, request)
        
    def _process_query_result(self, request):
        """Process result"""
        query_name = request['data'][0]['name']
        logger.info("Received result for query %s", query_name)
        query_dir = os.path.join(self.results, query_name)
        self._save_results(query_dir, request['data'])

    def _reply(self, response):
        """Reply"""
        logger.info("Replying: %s", (str(response)))
        encoded_response = json.dumps(response).encode()
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.send_header('Content-length', len(encoded_response))
        self.end_headers()
        self.wfile.write(encoded_response)


class MockDarkBytesServer:
    """Mock DarkBytes class"""
    def __init__(self, server_root_dir=DARK_BYTES_SERVER_DIR, tls=True, port=10023, use_enrollment=True):
        self.server_root_dir = server_root_dir
        self.tls = tls
        self.port = port
        self.use_enrollment = use_enrollment
        self.configure_server_root_dir()
        self.cert_path = os.path.join(self.server_root_dir, 'server.crt')
        self.key_path = os.path.join(self.server_root_dir, 'server.key')
        self.enrollment_secret_path = os.path.join(self.server_root_dir, 'enrollment_secret.txt')
        self.enroll_secret = None
        self.httpd = None

    def __enter__(self):
        """Allow with statement"""
        self.run_thread = Thread(target=self.run_server)
        self.run_thread.start()
        return self

    def __exit__(self, *args):
        """Cleanup resource handles"""
        if self.run_thread.is_alive():
            self.stop_server()
            self.run_thread.join()

    def configure_server_root_dir(self):
        """Create server root directory"""
        os.makedirs(self.server_root_dir, exist_ok=True)

        # Copy certificates and configuration files to the root server directory
        shutil.copy(os.path.join(CURRENT_DIRECTORY, 'config.json'), os.path.join(self.server_root_dir, 'config.json'))
        enroll_path = os.path.join(CURRENT_DIRECTORY, 'enrollment_secret.txt')
        if os.path.exists(enroll_path):
            shutil.copy(enroll_path,
                    os.path.join(self.server_root_dir, "enrollment_secret.txt"))
        else:
            logger.info("It will accept any enrollment_secret while running")
        shutil.copy(os.path.join(CURRENT_DIRECTORY, 'certificates', 'server.crt'),
                    os.path.join(self.server_root_dir, "server.crt"))
        shutil.copy(os.path.join(CURRENT_DIRECTORY, 'certificates', 'server.key'),
                    os.path.join(self.server_root_dir, "server.key"))

    def run_server(self):
        """Run the http server"""
        logger.info("binding to port %s", self.port)

        if self.use_enrollment:
            if os.path.exists(self.enrollment_secret_path):
                with open(self.enrollment_secret_path, "r") as file:
                    self.enroll_secret = file.read().strip()
            else:
                logger.info("Enrollment secred not present.It will accept any enronllment secret.")
                self.enroll_secret = ACCEPT_ANY_ENROLMENT
            ENROLL_RESPONSE['node_key'] = self.enroll_secret

        simple_handler = partial(RealSimpleHandler, self.server_root_dir, self.enroll_secret)
        self.httpd = HTTPServer(('localhost', self.port), simple_handler)
        if self.tls:
            if 'SSLContext' in vars(ssl):
                ctx = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
                ctx.load_cert_chain(self.cert_path, keyfile=self.key_path)
                ctx.options ^= ssl.OP_NO_SSLv2 | ssl.OP_NO_SSLv3
                self.httpd.socket = ctx.wrap_socket(self.httpd.socket, server_side=True)
            else:
                self.httpd.socket = ssl.wrap_socket(
                    self.httpd.socket,
                    ssl_version=ssl.PROTOCOL_SSLv23,
                    certfile=self.cert_path,
                    keyfile=self.key_path,
                    server_side=True)
            logger.info("Starting TLS/HTTPS server on TCP port: %s", self.port)
        else:
            logger.info("Starting HTTP server on TCP port: %s", self.port)
        try:
            self.httpd.serve_forever()
        except KeyboardInterrupt:
            sys.exit(0)
        finally:
            self.httpd.server_close()

    def stop_server(self):
        logger.info("Terminating TLS/HTTPS server on TCP port: %s", self.port)
        self.httpd.shutdown()


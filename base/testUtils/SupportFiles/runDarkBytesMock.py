#!/usr/bin/env python3
import logging
import sys
import os
import mock_darkbytes

DARK_BYTES_SERVER_DIR = r'/tmp/test/mock_darkbytes'
log_path = os.path.join(DARK_BYTES_SERVER_DIR, 'osquery_server.log')
mock_darkbytes.DARK_BYTES_SERVER_DIR = DARK_BYTES_SERVER_DIR

if os.path.exists(log_path):
    os.remove(log_path)

file_handler = logging.FileHandler(filename=log_path)

handlers = [file_handler]

logging.basicConfig(handlers=handlers, level=logging.DEBUG,
                    format='%(asctime)s %(levelname)-8s%(message)s', datefmt='%Y-%m-%d %H:%M:%S')

m = mock_darkbytes.MockDarkBytesServer(port=443)
m.run_server()

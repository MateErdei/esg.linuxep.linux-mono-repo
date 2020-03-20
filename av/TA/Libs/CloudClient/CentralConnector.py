#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import base64
import errno
import json
import os
import re
import ssl
import time

import urllib.error
import urllib.request

try:
    from robot.api import logger
    logger.warning = logger.warn
except ImportError:
    import logging
    logger = logging.getLogger(__name__)

try:
    from . import SophosHTTPSClient
    from . import SophosRobotSupport
except ImportError:
    import SophosHTTPSClient
    import SophosRobotSupport

wrap_ssl_socket = SophosHTTPSClient.wrap_ssl_socket
set_ssl_context = SophosHTTPSClient.set_ssl_context
_add_handler = SophosHTTPSClient._add_handler
set_proxy = SophosHTTPSClient.set_proxy
raw_request_url = SophosHTTPSClient.raw_request_url
request_url = SophosHTTPSClient.request_url
json_loads = SophosHTTPSClient.json_loads

GL_REGION_URL = {
    'p': 'https://cloud.sophos.com',
    'q': 'https://p0.q.hmr.sophos.com',
    'DEV': 'https://p0.d.hmr.sophos.com',
    'sb': 'https://fe.sandbox.sophos',
    'linux': 'https://linux.cloud.sandbox',
}


def _getUrl(region):
    return GL_REGION_URL[region]


def _getUsername(region):
    return "douglas.leeder@sophos.com"


def _getPassword(region):
    return "Ch1pm0nk"


def _get_my_hostname(hostname=None):
    if hostname is None:
        hostname = os.uname()[1]

    return hostname.split(".")[0]


class CloudClientException(Exception):
    pass


class HostMissingException(CloudClientException):
    pass


class Options(object):
    pass

class CentralConnector(object):
    def __init__(self, region):
        self.__m_region = region
        self.__m_url = _getUrl(region)
        self.__m_username = _getUsername(region)
        self.__m_password = _getPassword(region)
        self.default_headers = {}
        self.__m_my_hostname = _get_my_hostname()

        ## To match regression suite cloudClient.py
        self.options = Options()
        self.options.email = self.__m_username
        self.options.password = self.__m_password
        self.host = self.__m_url

    def __tmp_root(self):
        return SophosRobotSupport.TMPROOT()

    def __loadExistingCredentials(self):
        email = self.__m_username
        password = self.__m_password

        # POST request to get the Hammer token.
        token = email.strip() + ':' + password
        credential = base64.b64encode(token.encode("UTF-8"))

        TMPROOT = self.__tmp_root()
        try:
            with open(os.path.join(TMPROOT, "cloud_tokens.txt"), "rb") as infile:
                session_data = json.load(infile)
            if session_data['host'] == self.__m_url and session_data['credential'] == credential:
                self.default_headers['X-Hammer-Token'] = session_data['hammer_token']
                self.default_headers['X-CSRF-Token'] = session_data['csrf_token']
                self.upe_api = session_data['upe_url']
                logger.debug("Using cached cloud login")
        except IOError:
            pass

    def __retry_request_url(self, request):
        if self.upe_api is None:
            self.login(request)

        assert 'X-Hammer-Token' in self.default_headers
        assert 'X-CSRF-Token' in self.default_headers

        #~ print(request.header_items(), file=sys.stderr)

        assert request.has_header('X-hammer-token') ## urllib2 title cases the header
        assert request.has_header('X-csrf-token') ## urllib2 title cases the header
        assert self.upe_api is not None

        attempts = 0
        delay = 0.5
        reason = None
        while attempts < 5:
            attempts += 1
            try:
                return request_url(request)
            except urllib.error.HTTPError as e:
                reason = e.reason
                if e.code == 403:
                    if attempts > 1:
                        print("403 from request - logging in", file=sys.stderr)
                        self.login(request)
                    else:
                        print("403 from request - retrying", file=sys.stderr)
                elif e.code == 401:
                    print("401 from request - Cached session timed out - logging in", file=sys.stderr)
                    self.login(request)
                else:
                    raise
            except urllib.error.URLError as e:
                reason = e.reason
                print("urllib.error.URLError - retrying", file=sys.stderr)
                if e.errno == errno.ECONNREFUSED:
                    delay += 5 ## Increase the delay to handle a connection refused
            except ssl.SSLError as e:
                reason = str(e)
                print("ssl.SSLError - retrying", file=sys.stderr)

            time.sleep(delay)
            delay += 1

        if reason is None:
            raise CloudClientException("Failed to {method} {url}".format(url=request.get_full_url(), method=request.get_method()))
        else:
            raise CloudClientException("Failed to {method} {url}: {reason}".format(
                url=request.get_full_url(),
                method=request.get_method(),
                reason=reason
            ))

    def login(self, request=None):
        # Authenticate into cloud account for subsequent requests.

        email = self.options.email
        password = self.options.password

        # POST request to get the Hammer token.
        token = email.strip() + ':' + password
        credential = base64.b64encode(token.encode("UTF-8"))

        logger.debug("Logging in as %s at %s" % (email, self.host))

        self.default_headers.pop('X-Hammer-Token', None)
        self.default_headers.pop('X-CSRF-Token', None)

        zero_request = urllib.request.Request(self.host + '/api/sessions', headers=self.default_headers)
        zero_request.get_method = lambda: 'POST'
        zero_request.add_header('Authorization', b'Hammer ' + credential)

        try:
            zero_response = request_url(zero_request)
        except urllib.error.HTTPError as e:
            if e.code == 403:
                ## Retry 403 responses
                logger.error("403 from login - retrying")
                zero_response = request_url(zero_request)
            else:
                raise

        json_response = json_loads(zero_response)

        #parse session response
        self.upe_api = json_response['apis']['upe']['ng_url']
        hammer_token = json_response['token']

        csrf_token = json_response.get("csrf",None)

        dashboard_response = None
        if csrf_token is None:
            logger.warning("WARNING: Failed to get csrf_token from zero request, fetching dashboard")
            dashboard_request = urllib.request.Request(self.host)
            dashboard_response = request_url(dashboard_request)
            csrf_token = re.findall(r'HAMMER_CSRF_TOKEN\s*=\s*[\'"]([^\'"]+)[\'"]', dashboard_response)[0]
            if not csrf_token:
                raise Exception("HAMMER_CSRF_TOKEN not found in markup at /dashboard")

        self.default_headers['X-Hammer-Token'] = str(hammer_token)
        self.default_headers['X-CSRF-Token'] = csrf_token
        if request is not None:
            request.add_header('X-Hammer-Token', str(hammer_token))
            request.add_header('X-CSRF-Token', csrf_token)

            ## need to remove old Cookie headers, CookieJar won't update them
            request.headers.pop('Cookie', None)
            request.unredirected_hdrs.pop('Cookie', None)

        ## save the pertinent session data for reuse by subsequent cloudClient calls
        session_data = {
            'host': self.host,
            'credential': credential,
            'hammer_token': hammer_token,
            'csrf_token': csrf_token,
            'upe_url': self.upe_api,
        }
        TMPROOT = self.__tmp_root()
        with open(os.path.join(TMPROOT, "cloud_tokens.txt"), "wb") as outfile:
            json.dump(session_data, outfile)

        #~ logger.debug(session_data)

        if len(csrf_token) not in (32, 36, 64):
            logger.error("Unexpected CSRF token length=%d", len(csrf_token))
            if os.environ.get("CLOUD_VERBOSE", "0") == "1":
                logger.error("self.default_headers: {}".format(self.default_headers))
                logger.error("response: {}".format(json.dumps(json_response)))
            open(os.path.join(TMPROOT, "cloud-automation-json.dump"), "w").write(json.dumps(json_response))
            if dashboard_response is not None:
                open(os.path.join(TMPROOT, "cloud-automation-raw.html"), "w").write(dashboard_response)

    def getCloudVersion(self):
        dashboard_request = urllib.request.Request(self.host)
        dashboard_response = request_url(dashboard_request)
        version = re.findall(r'<!-- Sophos Central Version: ([0-9.]*) -->', dashboard_response)[0]
        if not version:
            raise Exception("Sophos Central Version not found in markup at /dashboard")
        return version

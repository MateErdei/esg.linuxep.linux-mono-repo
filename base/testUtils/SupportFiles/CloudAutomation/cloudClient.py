#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Derived from Perforce //dev/savlinux/regression/supportFiles/CloudAutomation/cloudClient.py

import os
import time
import calendar
import getpass
import re
import base64
import json
from optparse import OptionParser
import ssl
import socket
import traceback
import errno
import sys
import logging
import http.client

import urllib.request
from urllib.parse import urlencode

try:
    from . import SophosHTTPSClient
except (ValueError, ImportError):
    import SophosHTTPSClient

VERBOSE = False

DEFAULT_CIPHERS = (
    'ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:'
    'ECDH+HIGH:DH+HIGH:ECDH+3DES:DH+3DES:RSA+AESGCM:RSA+AES:RSA+HIGH:'
    'RSA+3DES:ECDH+RC4:DH+RC4:RSA+RC4:-aNULL:-eNULL:-EXP:-MD5:RSA+RC4+MD5'
)

logger = logging.getLogger("cloudCloud")

socket.setdefaulttimeout(60)

wrap_ssl_socket = SophosHTTPSClient.wrap_ssl_socket
set_ssl_context = SophosHTTPSClient.set_ssl_context
_add_handler = SophosHTTPSClient._add_handler
set_proxy = SophosHTTPSClient.set_proxy
raw_request_url = SophosHTTPSClient.raw_request_url
request_url = SophosHTTPSClient.request_url
json_loads = SophosHTTPSClient.json_loads

def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't even have to be reachable
        s.connect(('10.255.255.255', 1))
        IP = s.getsockname()[0]
    except:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP

def host(hostname=None):
    if hostname is None or hostname == "HOSTNAME":
        hostname = os.uname()[1]

    return hostname.split(".")[0]


class CloudClientException(Exception):
    pass


class HostMissingException(CloudClientException):
    pass


class CloudClient(object):
    # ensure that we're using AJAX requests
    default_headers = {
        'X-Requested-With': 'XMLHttpRequest',
        'Content-Type': 'application/json; charset=utf-8',
        'Accept': 'application/json, text/javascript, */*',
    }

    hosts = {
        'p': 'https://cloud.sophos.com',
        'q': 'https://p0.q.hmr.sophos.com',
        'd': 'https://p0.d.hmr.sophos.com',
        'sb': 'https://fe.sandbox.sophos',
        'linux': 'https://linux.cloud.sandbox',
    }

    def __init__(self, options):
        self.options = options
        self.host = self.hosts.get(self.options.region, self.options.region)

        hostname = self.host.split("://")[1]

        # fix up SSL
        wrap_ssl_socket(hostname)

        set_proxy(self.options.proxy, self.options.proxy_username, self.options.proxy_password)

        set_ssl_context(True)
        self.upe_api = None
        self.loadExistingCredentials()

        if self.upe_api is None:
            self.login()

    def isConfiguredWithUpdateCache(self):
        # In Central Utils; CENTRAL_CONFIG->ucmr uses the email below
        return self.options.email == "trunk@savlinux.xmas.testqa.com"


    def loadExistingCredentials(self):
        email = self.options.email
        password = self.options.password

        # POST request to get the Hammer token.
        token = email.strip() + ':' + password
        credential = base64.b64encode(token.encode("UTF-8"))

        TMPROOT = os.environ.get("TMPROOT", "/tmp")

        try:
            with open(os.path.join(TMPROOT, "cloud_tokens.txt"), "rb") as infile:
                session_data = json.load(infile)
            if session_data['host'] == self.host and session_data['credential'] == credential:
                self.default_headers['X-Hammer-Token'] = session_data['hammer_token']
                self.default_headers['X-CSRF-Token'] = session_data['csrf_token']
                self.upe_api = session_data['upe_url']
                print("Using cached cloud login", file=sys.stderr)
        except IOError:
            pass

    def retry_request_url(self, request):
        if self.upe_api is None:
            self.login(request)

        assert 'X-Hammer-Token' in self.default_headers
        assert 'X-CSRF-Token' in self.default_headers

        # ~ print(request.header_items(), file=sys.stderr)

        assert request.has_header('X-hammer-token')  ## urllib.request title cases the header
        assert request.has_header('X-csrf-token')  ## urllib.request title cases the header
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
                print("urllib.request.URLError - retrying", file=sys.stderr)
                if e.errno == errno.ECONNREFUSED:
                    delay += 5  ## Increase the delay to handle a connection refused
            except ssl.SSLError as e:
                reason = str(e)
                print("ssl.SSLError - retrying", file=sys.stderr)

            time.sleep(delay)
            delay += 1

        if reason is None:
            raise Exception("Failed to {method} {url}".format(url=request.get_full_url(), method=request.get_method()))
        else:
            raise Exception("Failed to {method} {url}: {reason}".format(
                url=request.get_full_url(),
                method=request.get_method(),
                reason=reason
            ))

    def login(self, request=None):
        # Authenticate into cloud account for subsequent requests.

        email = self.options.email
        password = self.options.password
        dashboard_response = None

        # POST request to get the Hammer token.
        token = email.strip() + ':' + password
        credential = base64.b64encode(token.encode("UTF-8"))

        print("Logging in as %s at %s" % (email, self.host), file=sys.stderr)

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
                print("403 from login - retrying", file=sys.stderr)
                zero_response = request_url(zero_request)
            else:
                raise

        json_response = json_loads(zero_response)

        # parse session response
        self.upe_api = json_response['apis']['upe']['ng_url']
        hammer_token = json_response['token']

        csrf_token = json_response.get("csrf", None)

        if csrf_token is None:
            print("WARNING: Failed to get csrf_token from zero request, fetching dashboard", file=sys.stderr)
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
            'credential': credential.decode("utf-8"),
            'hammer_token': hammer_token,
            'csrf_token': csrf_token,
            'upe_url': self.upe_api,
        }
        TMPROOT = os.environ.get("TMPROOT", "/tmp/PeanutIntegration")
        if not os.path.exists(TMPROOT):
            os.makedirs(TMPROOT)

        with open(os.path.join(TMPROOT, "cloud_tokens.txt"), "w", encoding="utf8") as outfile:
            json.dump(session_data, outfile)

        # ~ print(session_data, file=sys.stderr)

        if len(csrf_token) not in (32, 36, 64):
            print("Unexpected CSRF token length=%d" % len(csrf_token), file=sys.stderr)
            if os.environ.get("CLOUD_VERBOSE", "0") == "1":
                print("self.default_headers: {}".format(self.default_headers), file=sys.stderr)
                print("response: {}".format(json.dumps(json_response)), file=sys.stderr)
            tmp = os.environ.get("TMPROOT", "/tmp")
            open(os.path.join(tmp, "cloud-automation-json.dump"), "w").write(json.dumps(json_response))
            if dashboard_response is not None:
                open(os.path.join(tmp, "cloud-automation-raw.html"), "w").write(dashboard_response)

    def _getItems(self, response):
        return json_loads(response)["items"]

    def month_string_to_int(self, month_string):
        try:
            months = {"jan": 1,
                      "feb": 2,
                      "mar": 3,
                      "apr": 4,
                      "may": 5,
                      "jun": 6,
                      "jul": 7,
                      "aug": 8,
                      "sep": 9,
                      "oct": 10,
                      "nov": 11,
                      "dec": 12}
            return months[month_string[:3].lower()]
        except:
            raise KeyError("month_string_to_int was not provided with a valid month_string")


    def getCloudTime(self):
        login_request = urllib.request.Request(self.host + '/login')
        login_response = raw_request_url(login_request)

        cloud_time = login_response.info().get("Date")

        dow, dom, month_string, year, timestamp24h, GMT = cloud_time.split(" ")
        hour, minute, second = timestamp24h.split(":")
        month = self.month_string_to_int(month_string)

        cloud_time_tuple = (int(year), int(month), int(dom), int(hour), int(minute), int(second))

        # time.mktime uses localtime, calendar.timegm uses utc
        return float(calendar.timegm(cloud_time_tuple))

    def getCloudVersion(self):
        dashboard_request = urllib.request.Request(self.host)
        dashboard_response = request_url(dashboard_request)
        version = re.findall(r'<!-- Sophos Central Version: ([0-9.]*) -->', dashboard_response)[0]
        if not version:
            raise Exception("Sophos Central Version not found in markup at /dashboard")
        return version

    def getUsers(self):
        upe_users = self.upe_api + "/directory/users"
        upe_user_request = urllib.request.Request(upe_users, headers=self.default_headers)
        upe_user_response = self.retry_request_url(upe_user_request)

        return self._getItems(upe_user_response)

    def getDevices(self, limit=None, offset=None, q=None):
        url = self.upe_api + "/user-devices"
        query = {}
        if limit is not None:
            query["limit"] = limit
        if offset is not None:
            query["offset"] = offset
        if q is not None:
            query["q"] = q
        if any(query):
            url += "?" + urlencode(query)

        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)

        return self._getItems(response)

    def getDeviceByName(self, name):
        devices = self.getDevices(q=name)
        devices = [device for device in devices if device['name'] == name]

        if len(devices) == 1:
            return devices[0]
        elif len(devices) > 1:
            raise Exception("Too many matching devices")
        else:
            return None

    def getDeviceById(self, device_id):
        url = self.upe_api + "/user-devices/" + device_id
        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)

        return json_loads(response)

    ## ServersController (/api/servers)

    def getServers(self, limit=None, offset=None, q=None):
        url = self.upe_api + "/servers"
        query = {}
        if limit is not None:
            query["limit"] = limit
        if offset is not None:
            query["offset"] = offset
        if q is not None:
            query["q"] = q
        if any(query):
            url += "?" + urlencode(query)

        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)

        return self._getItems(response)

    def getServerId(self, hostname=None):
        if hostname is None:
            hostname = self.options.hostname

        hostname = host(hostname)
        servers = self.getServers(q=hostname)
        # Below line is required if one hostname is a subset of another
        ## e.g. abelard, abelardRHEL67vm
        servers = [s for s in servers if s['name'] == hostname]

        if len(servers) == 0:
            serverList = []
            for s in self.getServers():  ## Check all servers just in case
                serverList.append(s['name'])
                if s['name'] == hostname:
                    return s['id']
            print("No server found with name %s, Found: [%s]" % (hostname, str(serverList)), file=sys.stderr)
            return None

        if len(servers) > 1:

            ip = get_ip()
            for server in servers:
                ipaddresses = server['info']['ipAddresses/ipv4']

                for ipaddr in ipaddresses:
                    if ipaddr == ip:

                        serverId = server['id']
                        return serverId

            ## dump all of the servers found
            print(json.dumps(servers, indent=2, sort_keys=True), file=sys.stderr)
            raise Exception("Multiple servers of name %s found: %d" % (hostname, len(servers)))

        serverId = servers[0]['id']
        return serverId

    def getServerByName(self, hostname=None):
        serverId = self.getServerId(hostname)
        if serverId is None:
            return None
        return self.getServerById(serverId)

    def getServerById(self, serverId=None):
        if serverId is None:
            raise Exception("Invalid machine ID")

        url = self.upe_api + "/servers/" + str(serverId)
        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)
        return json_loads(response)

    def getMachineId(self, hostname=None):
        server = self.getServerByName(hostname)
        return server["machine_id"]

    def deleteServerByName(self, hostname=None):
        """
        https://api.sandbox.sophos/api/servers/7b6e88f5-28ca-14d5-ab97-e3ec48bcca2a
        """
        hostname = host(hostname)
        servers = self.getServers(q=hostname)
        # Below line is required if one hostname is a subset of another
        ## e.g. abelard, abelardRHEL67vm
        servers = [s for s in servers if s['name'] == hostname]

        if len(servers) > 1:
            print("Deleting multiple servers", file=sys.stderr)
        elif len(servers) == 0:
            print("No server for %s" % hostname, file=sys.stderr)
            return ""

        res = ""
        for server in servers:
            res = self.deleteServerById(server['id'])

        return res

    def deleteServerById(self, server_id):
        url = self.upe_api + "/servers/" + server_id
        # ~ print("url: %s"%url,file=sys.stderr)
        # ~ print("headers: {}".format(self.default_headers),file=sys.stderr)
        request = urllib.request.Request(url, headers=self.default_headers)
        request.get_method = lambda: 'DELETE'
        response = self.retry_request_url(request)
        return response

    def scanNow(self, hostname):
        server = self.getServerByName(hostname)
        if server is None:
            return
        url = self.upe_api + "/action/scan_now"
        data = json.dumps({"endpoint_id": server['id']}).encode('UTF-8')
        request = urllib.request.Request(url, data=data, headers=self.default_headers)
        response = self.retry_request_url(request)
        return response

    def updateNow(self, hostname=None):
        # https://upe.linux.cloud.sandbox/frontend/api/action/alc_force_update
        # {"endpoint_id":"76e57dc0-3115-d4f6-49d8-13e16d11e643"}

        # https://dzr-api-amzn-eu-west-1-f9b7.api-upe.d.hmr.sophos.com/api/action/alc_force_update
        # https://amzn-eu-west-1-f9b7.api-upe.d.hmr.sophos.com/frontend/api/action/alc_force_update
        server = self.getServerByName(hostname)
        if server is None:
            return
        url = self.upe_api + "/action/alc_force_update"
        data = json.dumps({"endpoint_id": server['id']}, separators=(',', ': ')).encode('UTF-8')
        request = urllib.request.Request(url, data=data, headers=self.default_headers)
        response = self.retry_request_url(request)
        return response

    def checkEngineVersion(self, expectedEngineVersion):
        actualEngineVersion = None
        hostname = self.options.hostname
        server = self.getServerByName(hostname)
        if server is None:
            print("hostname %s not found" % self.options.hostname, file=sys.stderr)
            return 1
        assert server['name'] == hostname
        assert 'id' in server

        start = time.time()
        delay = 1
        while time.time() < start + self.options.wait:
            actualEngineVersion = server['status']['sav/virus_engine_version']
            if expectedEngineVersion == actualEngineVersion:
                return True
            time.sleep(delay)
            delay += 1
            server = self.getServerById(server['id'])

        print("Engine doesn't match, expected " + expectedEngineVersion + " but got " + actualEngineVersion)
        return False

    def checkVDLVersion(self, expectedVDLVersion):
        actualVDLVersion = None
        hostname = self.options.hostname
        server = self.getServerByName(hostname)
        if server is None:
            print("hostname %s not found" % self.options.hostname, file=sys.stderr)
            return 1
        assert server['name'] == hostname
        assert 'id' in server

        start = time.time()
        delay = 1
        while time.time() < start + self.options.wait:
            actualVDLVersion = server['status']['sav/virus_data_version']
            if expectedVDLVersion == actualVDLVersion:
                return True
            time.sleep(delay)
            delay += 1
            server = self.getServerById(server['id'])

        print("Data doesn't match, expected " + expectedVDLVersion + " but got " + actualVDLVersion)
        return False

    def checkServerDetails(self, expectedOsName, *expectedIPv4s, **awsArgs):
        hostname = self.options.hostname
        server = self.getServerByName(hostname)
        expectedOsName = expectedOsName.strip()
        if server is None:
            print("hostname %s not found" % self.options.hostname, file=sys.stderr)
            return 1

        assert server['name'] == hostname

        serverInfo = server['info']
        actualOsName = serverInfo['osName']

        def matchOsName(expectedOsName, actualOsName):
            if expectedOsName == actualOsName:
                return True
            expectedOsName = str(expectedOsName)
            actualOsName = str(actualOsName)
            if expectedOsName == actualOsName:
                return True
            actualOsName = actualOsName.replace("\u2215", "/")
            return expectedOsName == actualOsName

        if not matchOsName(expectedOsName, actualOsName):
            print("Distribution doesn't match: expected='%s'(%d chars) actual='%s'(%d chars)" % (
                expectedOsName, len(expectedOsName), actualOsName, len(actualOsName))
                  , file=sys.stderr)
            return 1

        actualIPv4s = serverInfo['ipAddresses/ipv4']
        expectedIPv4s = list(expectedIPv4s)
        for expected in expectedIPv4s:
            if expected not in actualIPv4s:
                print("Expected IPs not in Actual doesn't match:", expectedIPv4s, actualIPv4s, file=sys.stderr)
                return 1

        if awsArgs:
            awsInfo = server['awsInfo']
            actualValue = serverInfo.get('aws/instance_id', None)
            expectedValue = awsArgs.get('expectedAwsInstanceId', None)

            if actualValue is None and not expectedValue:
                ## Not on AWS
                return 0
            elif actualValue is None:
                print("SophosCentral didn't report an AWS instanceId: expected=", expectedValue, file=sys.stderr)
                return 1
            elif actualValue != expectedValue:
                print("Different instanceIds: expected=", expectedValue, "actual=", actualValue, file=sys.stderr)
                return 1

            actualValue = awsInfo.get('region_name', None)
            expectedValue = awsArgs.get('expectedAwsRegion', None)

            if actualValue is None and not expectedValue:
                ## Not on AWS
                return 0
            elif actualValue is None:
                print("SophosCentral didn't report an AWS Region: expected=", expectedValue, file=sys.stderr)
                return 1
            elif actualValue != expectedValue:
                print("Different Regions: expected=", expectedValue, "actual=", actualValue, file=sys.stderr)
                return 1

            actualValue = awsInfo.get('account_id', None)
            expectedValue = awsArgs.get('expectedAwsAccountId', None)

            if actualValue is None and not expectedValue:
                ## Not on AWS
                return 0
            elif actualValue is None:
                print("SophosCentral didn't report an AWS AccountId: expected=", expectedValue, file=sys.stderr)
                return 1
            elif actualValue != expectedValue:
                print("Different AccountIds: expected=", expectedValue, "actual=", actualValue, file=sys.stderr)
                return 1

        return 0

    def checkServerDetailsWithAwsInfo(self, expectedOsName, expectedAwsInstanceId, expectedAwsRegion,
                                      expectedAwsAccountId, expectedIPv4s):
        kwargs = {"expectedAwsInstanceId": expectedAwsInstanceId,
                  "expectedAwsRegion": expectedAwsRegion,
                  "expectedAwsAccountId": expectedAwsAccountId}

        start = time.time()
        delay = 1
        while time.time() < start + self.options.wait:
            if self.checkServerDetails(expectedOsName, expectedIPv4s, **kwargs) == 0:
                return True
            time.sleep(delay)
            delay += 1
        return False

    def checkServerUpToDateState(self, expectedUpToDateStateValue):
        actualUpToDateStateValue = None
        hostname = self.options.hostname
        server = self.getServerByName(hostname)
        if server is None:
            print("hostname %s not found" % self.options.hostname, file=sys.stderr)
            return 1
        assert server['name'] == hostname
        assert 'id' in server

        start = time.time()
        delay = 1
        while time.time() < start + self.options.wait:
            actualUpToDateStateValue = server['status']['sav/up_to_date_state']
            actual = int(actualUpToDateStateValue)
            if actual == int(expectedUpToDateStateValue):
                return 0
            time.sleep(delay)
            delay += 1
            server = self.getServerById(server['id'])
        print("Up to date state doesn't match:", expectedUpToDateStateValue, actualUpToDateStateValue, file=sys.stderr)
        return 1

    def checkServerLastSuccessUpdateTime(self, startTime):
        """
        Wait for last success update time to be after startTime
        """
        actualUpdateTime = None
        startTime = float(startTime)
        target = time.strftime("%Y-%m-%dT%H:%M:%S.000Z", time.gmtime(startTime))

        hostname = self.options.hostname
        server = self.getServerByName(hostname)
        if server is None:
            print("hostname %s not found" % self.options.hostname, file=sys.stderr)
            return 1
        assert server['name'] == hostname
        assert 'id' in server

        start = time.time()
        delay = 1
        while time.time() < start + self.options.wait:
            try:
                actualUpdateTime = server['status']['alc/last_successful_update']
            except KeyError:
                pass
            else:
                if actualUpdateTime >= target:
                    return 0
            time.sleep(delay)
            delay += 1
            server = self.getServerById(server['id'])

        print("Last successful update never became later than start time:", target, actualUpdateTime, file=sys.stderr)
        return 1

    ## server report

    def getServerReports(self, limit=None, offset=None, q=None):
        url = self.upe_api + "/reports/servers"
        query = {}
        if limit is not None:
            query["limit"] = limit
        if offset is not None:
            query["offset"] = offset
        if q is not None:
            query["q"] = q
        if any(query):
            url += "?" + urlencode(query)

        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)

        return self._getItems(response)

    def getServerReportByName(self, hostname):
        hostname = host(hostname)
        servers = self.getServerReports(q=hostname)
        servers = [s for s in servers if s['name'] == hostname]

        if len(servers) == 0:
            return None

        if len(servers) > 1:
            print("Multiple server reports found. ", file=sys.stderr)
            for idx, s in enumerate(servers):
                print("server: ", str(servers[idx]), file=sys.stderr)
            # Do not raise exception as the reports are all the same,
            # so returning the first one.
            # raise Exception("Multiple server reports for name %s found"%(hostname))
        return servers[0]

    def getServerPolicies(self, limit=None, offset=None, q=None):
        url = self.upe_api + "/policies/servers"
        query = {}
        if limit is not None:
            query["limit"] = limit
        if offset is not None:
            query["offset"] = offset
        if q is not None:
            query["q"] = q
        if any(query):
            url += "?" + urlencode(query)

        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)

        return json_loads(response)

    def deleteDisabledOrUnappliedPoliciesAndReturnGoodOne(self, policies):
        """
        Iterates through policies, deleting any that are disabled, or have no endpoints assigned to them.
        If only one remains then we return it.
        Otherwise we throw an exception
        """
        goodPolicy = None
        for policy in policies:
            if not policy['enabled'] or \
                    len(policy['applies_to']['endpoints']) == 0:
                self.deletePolicy(policy)
                continue

            print("Policy:", policy['id'], policy['category'], policy['applies_to']['endpoints'], policy,
                  file=sys.stderr)

            if goodPolicy is None:
                goodPolicy = policy
            # ~ else:
            # ~ print("Policy1:",goodPolicy['id'],goodPolicy['enabled'],goodPolicy['applies_to']['endpoints'],goodPolicy,file=sys.stderr)
            # ~ raise Exception("Multiple assigned, enabled policies of name %s found"%(policy['name']))

        return goodPolicy

    def getServerPolicyByName(self, name, category="threat_protection"):
        policies = self.getServerPolicies(q=name)
        if len(policies) > 1:
            policies = [p for p in policies if p['name'] == name]
        if len(policies) > 1:
            policies = [p for p in policies if p.get("category", category) == category]

        if len(policies) == 0:
            return None

        if len(policies) > 1:
            ## Delete all policies which manage no computers
            return self.deleteDisabledOrUnappliedPoliciesAndReturnGoodOne(policies)

        return policies[0]

    def enableServerPolicy(self, policy):
        if isinstance(policy, str):
            policy = self.getServerPolicyByName(policy)
        assert policy is not None

        url = self.upe_api + "/policies/servers/" + policy['id']
        data = json.dumps({"id": policy['id'], "enabled": True}).encode('UTF-8')
        request = urllib.request.Request(url, data=data, headers=self.default_headers)
        request.get_method = lambda: "PUT"
        response = self.retry_request_url(request)

        policy['enabled'] = True

        return json_loads(response)

    def cloneServerPolicy(self, originalname, newname, category="threat_protection"):
        original = self.getServerPolicyByName(originalname, category=category)
        assert original is not None

        url = self.upe_api + "/policies/servers/" + original['id'] + "/clone"

        data = json.dumps({"name": newname}).encode('UTF-8')  ## Name is ignored by server
        request = urllib.request.Request(url, data=data, headers=self.default_headers)

        response = self.retry_request_url(request)
        return json_loads(response)

    def getServerPolicy(self, hostname=None, category="threat_protection"):
        """
        Get the server policy for hostname, if it exists
        """
        hostname = host(hostname)
        policyname = "av" + hostname
        return self.getServerPolicyByName(policyname, category=category)


    def deleteUpdatingPolicyForHostname(self, hostname=None):
        hostname = host(hostname)
        endpointid = self.getServerId(hostname)
        if endpointid is None:
            raise HostMissingException()

        policyname = "av" + hostname
        policy = self.getServerPolicyByName(policyname, category="updating")

        if(policy):
            self.deletePolicy(policy)


    def ensureServerPolicy(self, hostname=None, category="threat_protection"):
        """
        Get the server policy, and ensure it exists
        """
        hostname = host(hostname)
        endpointid = self.getServerId(hostname)
        if endpointid is None:
            raise HostMissingException()

        policyname = "av" + hostname
        policy = self.getServerPolicyByName(policyname, category=category)
        if policy is None or policy['name'] != policyname:
            policy = self.cloneServerPolicy("Base Policy", policyname, category=category)
            assert policy is not None

        # rename and enable the new policy
        policy['name'] = policyname
        policy['enabled'] = True
        policy['base'] = False
        policy['applies_to']['endpoints'] = [endpointid]
        policy['applies_to']['groups'] = []
        policy['applies_to']['users'] = []
        policy['category'] = category

        return policy

    # Returns the sophos central ID of the current endpoint.
    def getMyEndpointId(self):
        server = self.getServerByName()
        if server is None:
            return None
        return server['id']

    # Deletes a paused update by ID.
    def deletePausedUpdate(self, pauseId):
        tosend = '{"ids": ["' + pauseId + '"]}'
        method = "DELETE"
        url = self.upe_api + "/dpu/subscriptions/server"
        request = urllib.request.Request(url, data=tosend, headers=self.default_headers)
        request.get_method = lambda: method
        response = self.retry_request_url(request)

    def setPausedUpdate(self, fixedVersion="10.999.1.2"):
        toSend = {
            "name": "PAUSE NAME",
            "description": "PAUSE DESCRIPTION",
            "actionOnExpire": "RECOMMENDED",
            "type": "SERVER",
            "componentSuites": [
                {
                    "longName": "Sophos Anti-Virus for Linux v9.12.9999 Component",
                    "shortName": "10.0.9999.0.0 COMPONENT SUITE SHORT NAME",
                    "rigidName": "5CF594B0-9FED-4212-BA91-A4077CB1D1F3",
                    "version": fixedVersion,
                    "components": [
                        {
                            "version": "2.0.0",
                            "longName": "Sophos Anti-Virus for Linux v9.12.9999 Component",
                            "shortName": "10.0.9999.0.0 COMPONENT SUITE SHORT NAME",
                            "rigidName": "5CF594B0-9FED-4212-BA91-A4077CB1D1F3"
                        }
                    ]
                }
            ]
        }

        method = "POST"
        url = self.upe_api + "/dpu/subscriptions"
        jsonToSend = json.dumps(toSend).encode('UTF-8')
        request = urllib.request.Request(url, data=jsonToSend, headers=self.default_headers)
        request.get_method = lambda: method
        response = self.retry_request_url(request)
        responseDict = json_loads(response)
        pausedSetId = responseDict['id']
        return pausedSetId

    def assignPausedUpdate(self, pausedSetId, endpointId=None):
        if endpointId is None:
            endpointId = self.getMyEndpointId()

        toSend = {
            "name": "test",
            "enabled": True,
            "target": "servers",
            "applies_to": {"users": [], "groups": [], "endpoints": [endpointId]},
            "expiry": {"enabled": False, "expires_at": "2450-01-01T00:00:00.000Z"},
            "settings": {"auto_update/enabled": True, "auto_update/paused_updating/enabled": True,
                         "auto_update/paused_updating/id": pausedSetId}
        }

        url = self.upe_api + "/policies/servers"
        method = "POST"
        jsonToSend = json.dumps(toSend).encode('UTF-8')
        request = urllib.request.Request(url, data=jsonToSend, headers=self.default_headers)
        request.get_method = lambda: method
        response = self.retry_request_url(request)
        return response

    def enableControlledUpdate(self):
        toSend = {
            "endpointType": "servers"
        }
        method = "POST"
        data = json.dumps(toSend).encode('UTF-8')
        url = self.upe_api + "/controlled-updates/pause"
        request = urllib.request.Request(url, data=data, headers=self.default_headers)
        request.get_method = lambda: method
        response = self.retry_request_url(request)
        return response

    def disableControlledUpdate(self):
        toSend = {
            "endpointType": "servers"
        }
        method = "POST"
        data = json.dumps(toSend).encode('UTF-8')
        url = self.upe_api + "/controlled-updates/resume"
        request = urllib.request.Request(url, data=data, headers=self.default_headers)
        request.get_method = lambda: method
        response = self.retry_request_url(request)
        return response

    def setServerPolicy(self, policy):
        if 'id' in policy:
            method = "PUT"
            url = self.upe_api + "/policies/servers/" + policy['id']
        else:
            method = "POST"
            url = self.upe_api + "/policies/servers"

        data = json.dumps(policy).encode('UTF-8')
        request = urllib.request.Request(url, data=data, headers=self.default_headers)
        request.get_method = lambda: method
        response = self.retry_request_url(request)
        return json_loads(response)

    def setAVPolicyBooleanOption(self, policy, option, value):
        enabled = value in [True, "True", "true", "1"]
        policy['settings'][option] = enabled

        return self.setServerPolicy(policy)

    def setAVPolicyListOption(self, policy, option, value):
        policy['settings'][option] = value

        return self.setServerPolicy(policy)

    def setScheduledScanExclusions(self, *exclusions):
        hostname = host()
        policy = self.ensureServerPolicy(hostname)
        assert policy is not None
        path = "malware/scheduled/"
        policy['settings'][path + "exclusions_enabled"] = len(exclusions) > 0
        policy['settings'][path + "posix_exclusions"] = exclusions
        return self.setServerPolicy(policy)

    def setScheduleScan(self, hostname, details, *exclusions):
        hostname = host(hostname)
        policy = self.ensureServerPolicy(hostname)
        assert policy is not None
        details = details.split(" ")
        path = "malware/scheduled/"

        enabled = details[0] in [True, "True", "true", "1"]
        policy['settings'][path + "enabled"] = enabled

        enabled = details[1] in [True, "True", "true", "1"]
        policy['settings'][path + "scan_archives"] = enabled

        policy['settings'][path + "time"] = details[2]

        days = details[3].split("-")
        days = [int(x) for x in days]
        policy['settings'][path + "days"] = days

        enabled = details[4] in [True, "True", "true", "1"]
        policy['settings'][path + "exclusions_enabled"] = enabled

        policy['settings'][path + "posix_exclusions"] = exclusions
        return self.setServerPolicy(policy)

    def setAVPolicy(self, hostname, option, value):
        hostname = host(hostname)
        policy = self.ensureServerPolicy(hostname)
        assert policy is not None

        # use_sophos_recommended needs to be false for any other policy setting to be applied
        policy['settings']['malware/use_sophos_recommended'] = False

        if option == "OnAccess":
            return self.setAVPolicyBooleanOption(policy, 'malware/on_access/enabled', value)
        elif option == "ExcludeRemoteFiles":
            return self.setAVPolicyBooleanOption(policy, "malware/exclude_remote_files", value)
        elif option == "LiveProtection":
            return self.setAVPolicyBooleanOption(policy, "malware/live-protection/enabled", value)
        elif option == "UploadSamples":
            return self.setAVPolicyBooleanOption(policy, "malware/sample-submission/enabled", value)
        elif option == "ScheduledScan":
            return self.setAVPolicyBooleanOption(policy, "malware/scheduled/enabled", value)
        elif option == "mtd":
            policy['settings']['malware/live-protection/enabled'] = True
            return self.setAVPolicyBooleanOption(policy, 'mtd/enabled', value)
        elif option == "heartbeat":
            return self.setAVPolicyBooleanOption(policy, 'heartbeat/protection_enabled', value)
        elif option == "recommended":
            return self.setAVPolicyBooleanOption(policy, 'malware/use_sophos_recommended', value)

    def setDelayedUpdatePolicy(self, hostname, day=None, time=None):
        hostname = host(hostname)
        policy = self.ensureServerPolicy(hostname, category="updating")
        assert policy is not None

        if day is not None:
            day = day.lower()
            day = {
                "sunday": 0,
                "monday": 1,
                "tuesday": 2,
                "wednesday": 3,
                "thursday": 4,
                "friday": 5,
                "saturday": 6,
            }.get(day, day)

            try:
                day = int(day)
            except ValueError:
                pass

        # ~ print ("setDelayedUpdatePolicy(day=%s, time=%s)"%(repr(day),repr(time)))

        # leave day and time blank to disable Delayed Updating
        if day is None or day == "" or time is None or time == "":
            policy["settings"]["auto_update/delay_updating/enabled"] = False
            policy["settings"]["auto_update/delay_updating/day"] = 0
            policy["settings"]["auto_update/delay_updating/time"] = "00:00"

        else:
            policy["settings"]["auto_update/delay_updating/enabled"] = True
            policy["settings"]["auto_update/delay_updating/day"] = day
            policy["settings"]["auto_update/delay_updating/time"] = time

        return self.setServerPolicy(policy)

    def getNTPPolicy(self, hostname, option):
        hostname = host(hostname)
        policy = self.ensureServerPolicy(hostname)
        assert policy is not None

        if option == "mtd":
            return policy['settings']['mtd/enabled']

        raise Exception("Unknown option %s" % option)

    def getAVPolicy(self, hostname, option):
        hostname = host(hostname)
        policy = self.ensureServerPolicy(hostname)
        assert policy is not None

        if option == "OnAccess":
            return policy['settings']['malware/on_access/enabled']

        raise Exception("Unknown option %s" % option)

    def setAVFieldEnabled(self, hostname, field, value):
        hostname = host(hostname)
        policy = self.ensureServerPolicy(hostname)
        assert policy is not None
        return self.setAVPolicyBooleanOption(policy, str(field), value)

    def setAVFieldList(self, hostname, field, *values):
        hostname = host(hostname)
        policy = self.ensureServerPolicy(hostname)
        assert policy is not None
        return self.setAVPolicyListOption(policy, str(field), values)

    def setExclusions(self, hostname, *values):
        hostname = host(hostname)
        policy = self.ensureServerPolicy(hostname)
        assert policy is not None
        paths = []
        for value in values:
            paths.append(self.__convertPathToUnicode(value, False))
        return self.setAVPolicyListOption(policy, "malware/on_access/posix_exclusions", paths)

    def addExclusion(self, hostname, value):
        hostname = host(hostname)
        policy = self.ensureServerPolicy(hostname)
        assert policy is not None
        encoded_value = self.__convertPathToUnicode(value, False)
        unampersanded_value = encoded_value.replace("&", "iamnothere")
        policy['settings']["malware/on_access/posix_exclusions"].append(unampersanded_value)
        # Doesn't send exclusions until the function below is called, in the anticipation
        # the caller is building up a list.
        return self.setServerPolicy(policy)

    def deleteAVPolicyByHostname(self):
        hostname = self.options.hostname
        policyname = "av" + hostname

        policies = self.getServerPolicies(q=policyname)
        policies = [p for p in policies if p['name'] == policyname]

        for policy in policies:
            self.deletePolicy(policy)

    def resetCloudAvPolicy(self):
        hostname = self.options.hostname
        policy = self.ensureServerPolicy(hostname)

        policy['settings']["malware/on_access/posix_exclusions"] = []
        policy['settings']['malware/on_access/enabled'] = True
        policy['settings']["malware/exclude_remote_files"] = True
        policy['settings']["malware/live-protection/enabled"] = True
        policy['settings']["malware/use_sophos_recommended"] = False

        policy['settings']["malware/scheduled/enabled"] = False
        policy['settings']["malware/scheduled/scan_archives"] = False
        policy['settings']["malware/scheduled/posix_exclusions"] = []

        return self.setServerPolicy(policy)

    def __isFakeCloud(self):
        # ~ logger.debug("%s",self.host)
        return self.host == "https://localhost:4443"

    def setOnaccessEnabled(self, hostname, value):
        hostname = host(hostname)

        if self.__isFakeCloud():
            logger.info("setOnaccessEnabled on fakeCloud")
            url = self.host + "/controller/onaccess/off"
            request = urllib.request.Request(url, headers=self.default_headers)
            response = self.retry_request_url(request)
        else:
            return self.setAVPolicy(hostname, "OnAccess", value)

    def setMtdEnabled(self, hostname, value):
        hostname = host(hostname)
        return self.setAVPolicy(hostname, "mtd", value)

    def setSophosRecommended(self, hostname, value):
        hostname = host(hostname)
        return self.setAVPolicy(hostname, "recommended", value)

    def getOnaccessEnabled(self, hostname):
        hostname = host(hostname)
        return self.getAVPolicy(hostname, "OnAccess")

    def getMtdEnabled(self, hostname):
        hostname = host(hostname)
        return self.getAVPolicy(hostname, "mtd")

    def generateAlerts(self, hostname=None, serverId=None, limit=None, batchLimit=100):
        """
        https://api.sandbox.sophos/api/alerts/endpoint/bbed18a9-4750-043e-d8a8-598da763f76c?limit=0&offset=0
        https://api.sandbox.sophos/api/alerts/endpoint/bbed18a9-4750-043e-d8a8-598da763f76c?endpointId=bbed18a9-4750-043e-d8a8-598da763f76c&limit=3&offset=0
        """
        if hostname is not None and serverId is None:
            hostname = host(hostname)
            serverId = self.getServerId(hostname)

        offset = 0
        numberProcessed = 0

        while True:
            if serverId:
                url = self.upe_api + "/alerts/endpoint/%s?limit=%d&offset=%d" % (serverId, batchLimit, offset)
            else:
                url = self.upe_api + "/alerts?limit=%d&offset=%d" % (batchLimit, offset)
            request = urllib.request.Request(url, headers=self.default_headers)
            response = self.retry_request_url(request)
            response = json_loads(response)
            total = response['total']
            items = response["items"]

            if len(items) == 0:
                return

            for alert in items:
                if hostname is not None and alert['location'] != hostname:
                    if serverId:
                        print(json.dumps(alert, indent=2), file=sys.stderr)
                        raise Exception(
                            "Incorrect hostname in alert: expected=%s, actual=%s" % (hostname, alert['location']))
                    else:
                        ## Can't expect to only have our alerts if we don't have a serverId
                        continue

                if serverId is not None and alert['data']['endpoint_id'] != serverId:
                    print(json.dumps(alert, indent=2), file=sys.stderr)
                    raise Exception("Incorrect serverId in alert: expected=%s, actual=%s" % (
                        serverId, alert['data']['endpoint_id']))

                numberProcessed += 1
                yield alert

                if limit is not None and numberProcessed >= limit:
                    ## consider raising an exception here?
                    print("WARNING: generateAlerts exceeded limit of %d items, fetched %d of %d items" % (
                        limit, numberProcessed, total), file=sys.stderr)
                    return

            if offset + len(items) == total:
                return
            offset += len(items)

    def getHealthFromFakeCloud(self, expectedHealth, hostname=None):
        hostname = host(hostname)
        url = self.upe_api + "/fake/health?hostname=%s" % (hostname)
        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)
        response = json_loads(response)
        health = response['health']
        print("received health: %s" % str(health))
        if health == expectedHealth:
            return 0
        return 1

    def waitForHealthFromFakeCloud(self, expectedHealth, hostname=None):
        hostname = host(hostname)
        url = self.upe_api + "/fake/health?hostname=%s" % (hostname)

        start = time.time()
        delay = 1
        while time.time() < start + self.options.wait:
            request = urllib.request.Request(url, headers=self.default_headers)
            response = self.retry_request_url(request)
            response = json_loads(response)
            health = response['health']
            if health == expectedHealth:
                return 0
            print("received health: %s, waiting for %s" % (str(health), str(expectedHealth)), file=sys.stderr)
            time.sleep(delay)
            delay += 1

        return 1

    def getUpdateSourceFromFakeCloud(self, expectedUpdateSource, hostname=None):
        hostname = host(hostname)
        url = self.upe_api + "/fake/updatesource?hostname=%s" % (hostname)
        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)
        response = json_loads(response)
        updatesource = response['updatesource']
        print("received updatesource: %s" % str(updatesource))
        if updatesource == expectedUpdateSource:
            return 0
        return 1

    def waitForUpdateSourceFromFakeCloud(self, expectedUpdateSource, hostname=None):
        hostname = host(hostname)
        url = self.upe_api + "/fake/updatesource?hostname=%s" % (hostname)

        start = time.time()
        delay = 1
        while time.time() < start + self.options.wait:
            request = urllib.request.Request(url, headers=self.default_headers)
            response = self.retry_request_url(request)
            response = json_loads(response)
            updatesource = response['updatesource']
            if updatesource == expectedUpdateSource:
                print("received expected updatesource: %s" % (str(updatesource)), file=sys.stderr)
                return 0
            print("received updatesource: %s, waiting for %s" % (str(updatesource), str(expectedUpdateSource)),
                  file=sys.stderr)
            time.sleep(delay)
            delay += 1

        return 1

    def clearAllAlerts(self, hostname=None):
        if hostname is None:
            hostname = self.options.hostname
        hostname = host(hostname)
        serverId = self.getServerId(hostname)
        print("clearAllAlerts hostname=%s serverId=%s" % (hostname, serverId), file=sys.stderr)
        url = self.upe_api + "/alerts/actions/clear"
        for iteration in range(20):
            alerts = self.generateAlerts(serverId=serverId, hostname=hostname)
            ## just the malware alerts
            alert_ids = [a['id'] for a in alerts]
            if len(alert_ids) == 0:
                return
            alert_ids = alert_ids[:100]
            print("Clearing %d alerts" % (len(alert_ids)), file=sys.stderr)

            data = json.dumps({'ids': alert_ids}).encode('UTF-8')
            request = urllib.request.Request(url, data=data, headers=self.default_headers)
            response = self.retry_request_url(request)

        raise Exception("clearAllAlerts failed to clear alerts")

    def clearMalwareAlerts(self, hostname=None):
        hostname = host(hostname)
        serverId = self.getServerId(hostname)
        url = self.upe_api + "/alerts/actions/clear"
        alerts = []

        for iteration in range(20):
            alerts = self.generateAlerts(serverId=serverId, hostname=hostname)
            ## just the malware alerts
            alert_ids = [a['id'] for a in alerts if a['threat'] is not None]
            if len(alert_ids) == 0:
                return

            alert_ids = alert_ids[:100]

            data = json.dumps({'ids': alert_ids}).encode('UTF-8')
            request = urllib.request.Request(url, data=data, headers=self.default_headers)
            response = self.retry_request_url(request)

        print("failed to clear alerts: %s" % (json.dumps(alerts, indent=2)), file=sys.stderr)
        raise Exception("clearMalwareAlerts failed to clear alerts")

    def resolveMalwareAlerts(self):
        hostname = self.options.hostname
        serverId = self.getServerId(hostname)
        url = self.upe_api + "/action/sav_clear_threats"
        alerts = []

        for iteration in range(20):
            alerts = self.generateAlerts(serverId=serverId, hostname=hostname)
            ## just the malware alerts
            alert_ids = [a['id'] for a in alerts if a['threat'] is not None]
            if len(alert_ids) == 0:
                return

            alert_ids = alert_ids[:100]

            data = json.dumps({'alert_ids': alert_ids}).encode('UTF-8')
            request = urllib.request.Request(url, data=data, headers=self.default_headers)
            response = self.retry_request_url(request)

        print("failed to resolve alerts: %s" % (json.dumps(alerts, indent=2)), file=sys.stderr)
        raise Exception("resolveMalwareAlerts failed to resolve alerts")

    def resolveMalwareAlertByFilename(self, filename):
        hostname = self.options.hostname
        serverId = self.getServerId(hostname)
        url = self.upe_api + "/action/sav_clear_threats"

        for iteration in range(10):
            for a in self.generateAlerts(serverId=serverId, hostname=hostname, limit=100):
                if filename in a['description']:
                    data = json.dumps({'alert_ids': [a['id']]}).encode('UTF-8')
                    request = urllib.request.Request(url, data=data, headers=self.default_headers)
                    response = self.retry_request_url(request)
                    return

        raise Exception("resolveMalwareAlertsByFilename failed to resolve alert for file %s", filename)

    def __convertPathToUnicode(self, path, addEncoding=True):
        if isinstance(path, str):
            return path
        if path is None:
            return path

        ## Try ASCII, UTF-8
        try:
            return str(path, b"utf-8")
        except UnicodeDecodeError:
            pass

        if addEncoding:
            ## Need to match the product code list
            encodings = ("EUC-JP", "SJIS", "Latin1")
        else:
            encodings = ("EUC-JP", "Latin1")

        for encoding in encodings:
            try:
                path = str(path, encoding)
                if addEncoding:
                    path += " (%s)" % encoding
                return path
            except UnicodeDecodeError:
                pass

        raise Exception("Unable to convert path to Latin1")

    def printAVAlerts(self):
        hostname = self.options.hostname
        serverId = self.getServerId(hostname)
        out = ["Alerts (limit=100):\n"]
        for alert in self.generateAlerts(serverId=serverId, hostname=hostname, limit=100):
            if 'description' in alert:
                description = alert['description']
            else:
                data = alert.get('data', {})
                description = data.get('description', "No description in alert['data'] or alert")
            out.append("%s: %s\n" % (description, str(alert)))

        out.append("Events:\n")
        url = self.upe_api + "/events?type=Event::Endpoint::Threat::Detected&actionable=false"

        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)
        out.append(response)

        return "".join(out)

    def waitForHealthStatus(self, overall_health, service_health, threat_health):
        status = {}
        machineID = self.getServerId(self.options.hostname)
        if machineID is None:
            print("NO SERVER NAMED %s FOUND" % self.options.hostname, file=sys.stderr)
            return 2

        start = time.time()
        delay = 1
        url = self.upe_api + "/servers/" + machineID + "?get_health_status=true"
        while time.time() < start + self.options.wait:
            request = urllib.request.Request(url, headers=self.default_headers)
            response = self.retry_request_url(request)
            response = json_loads(response)
            status = response['status']
            if (
                    overall_health == status.get('shs/health', None) and
                    service_health == status.get('shs/service', None) and
                    threat_health == status.get('shs/threat', None)
            ):
                return 0

            time.sleep(delay)
            delay += 1

        print(status.get('shs/health', "99") + " " + status.get('shs/service', "99") + " " + status.get('shs/threat',
                                                                                                        "99"))
        return 1

    def printHealthStatuses(self):
        """
        https://api.sandbox.sophos/api/servers/09bf6d94-0f6f-142b-9996-798ded029067?get_health_status=true
        """
        server = self.getServerByName(self.options.hostname)
        if server is None:
            print("NO SERVER NAMED %s FOUND" % self.options.hostname)
            return 1

        url = self.upe_api + "/servers/" + server['id'] + "?get_health_status=true"
        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)
        response = json_loads(response)
        status = response['status']
        try:
            print(status['shs/health'] + " " + status['shs/service'] + " " + status['shs/threat'])
        except KeyError:
            print('Key error - heartbeat not enabled?', file=sys.stderr)

    def printUtmId(self):
        """
        https://api.sandbox.sophos/api/servers/09bf6d94-0f6f-142b-9996-798ded029067?get_health_status=true
        """
        server = self.getServerByName(self.options.hostname)
        if server is None:
            print("NO SERVER NAMED %s FOUND" % self.options.hostname)

        url = self.upe_api + "/servers/" + server['id'] + "?get_health_status=true"
        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)
        response = json_loads(response)
        status = response['status']
        try:
            print(status['shs/active_heartbeat_utm_id'])
        except KeyError:
            print('Key error - heartbeat not enabled?', file=sys.stderr)

    def printMTDAlerts(self):
        hostname = self.options.hostname
        serverId = self.getServerId(hostname)
        out = []
        out.append("Alerts (limit=100):\n")
        for alert in self.generateAlerts(serverId=serverId, hostname=hostname, limit=100):
            if 'description' in alert:
                description = alert['description']
            else:
                data = alert.get('data', {})
                description = data.get('description', "No description in alert['data'] or alert")

            out.append("%s: %s\n" % (description, str(alert)))

        out.append("Events:\n")
        url = self.upe_api + "/events?type=Event::Endpoint::Threat::CommandAndControlDetected&actionable=false"

        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)
        out.append(response)

        return "".join(out)

    def getServerLicenseCodes(self, server_id=None):
        """
        https://api.sandbox.sophos/api/servers?get_health_status=true&get_license_codes=true
        """
        if server_id is None:
            server_id = self.getServerId(self.options.hostname)
            if server_id is None:
                raise HostMissingException()

        url = self.upe_api + "/servers/" + server_id + "?get_license_codes=true"
        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)
        response = json_loads(response)

        license_codes = response["license_codes"]
        return ",".join(license_codes)

    def checkServerLicenseCodes(self, expected):
        actual = None
        server_id = self.getServerId(self.options.hostname)
        if server_id is None:
            raise HostMissingException()

        expected_codes = set(expected.split(','))

        start = time.time()
        delay = 1
        while time.time() < start + self.options.wait:
            actual = self.getServerLicenseCodes(server_id=server_id)
            actual_codes = set(actual.split(','))

            if actual_codes == expected_codes:
                return 0
            time.sleep(delay)
            delay += 1

        print("Expected: {expected}, actual: {actual}".format(expected=expected, actual=actual), file=sys.stderr)
        return 1

    def checkMalwareAlert(self, hostname, description, location=None):
        hostname = host(hostname)
        serverId = self.getServerId(hostname)
        location = self.__convertPathToUnicode(location)
        start = time.time()
        delay = 1
        while time.time() < start + float(self.options.wait):
            for alert in self.generateAlerts(serverId=serverId, hostname=hostname):
                if alert['threat'] != description:
                    continue
                if location is not None and location not in alert['description']:
                    continue
                return 0

            time.sleep(delay)
            delay += 1
        return 1

    def checkMalwareAlerts(self, hostname, description, *locations):
        hostname = host(hostname)
        serverId = self.getServerId(hostname)

        locMap = {}
        locMapAll = {}
        for l in locations:
            l = self.__convertPathToUnicode(l)
            locMap[l] = 1
            locMapAll[l] = 1
        locationRE = re.compile(r".* at '([^']+)'")
        start = time.time()
        delay = 1
        while time.time() < start + self.options.wait:
            for alert in self.generateAlerts(serverId=serverId, hostname=hostname):
                if alert['threat'] != description:
                    continue

                mo = locationRE.match(alert['description'])
                if not mo:
                    print("RE didn't match:", alert['description'], file=sys.stderr)
                    continue

                location = self.__convertPathToUnicode(mo.group(1))
                if location in locMapAll:
                    locMap.pop(location, None)
                    if len(locMap) == 0:
                        return 0
                else:
                    print("Unexpected location: %s" % location, file=sys.stderr)

            time.sleep(delay)
            delay += 1
        missing = list(locMap.keys())
        missing.sort()
        for key in missing:
            print("Failed to find:", key, file=sys.stderr)
        return len(locMap)

    def checkMalwareAlertsFromFile(self, description, filename):
        paths = [x.strip() for x in open(filename).readlines()]
        return self.checkMalwareAlerts(None, description, *paths)

    TRUE = ["1", True, "True", "true", "T"]

    def checkStatusField(self, status, field, expected):
        TRUE = self.TRUE
        if field not in status:
            print("%s not found in status:" % (field), json.dumps(status, indent=2, sort_keys=True), file=sys.stderr)
            return False

        value = status[field]
        return (value in TRUE) == expected

    def waitForCompliancy(self, hostname, policyType="SAV", compliance=True):
        hostname = host(hostname)
        TRUE = self.TRUE
        compliance = compliance in TRUE
        key = {
            "SAV": "policy/SAV/2/non_compliant",
            "NTP": "policy/NTP/24/non_compliant",
        }[policyType]

        start = time.time()
        status = None
        delay = 0
        serverId = None
        while time.time() < start + self.options.wait:
            delay += 1
            if serverId is None:
                serverId = self.getServerId(hostname)
            if self.options.wait_for_host and serverId is None:
                time.sleep(delay)
                continue

            server = self.getServerById(serverId)
            assert server is not None
            status = server['status']

            if compliance:
                ## waiting to become compliant
                if key not in status:
                    return 0
                if not status[key]:
                    return 0
            else:
                ## waiting to become not compliant
                if key in status and status[key]:
                    return 0

            ## wanted state not matched
            time.sleep(delay)
            continue

        assert status is not None

        print("Last status received was:", json.dumps(status, indent=2, sort_keys=True), file=sys.stderr)

        return 1

    def waitForCompliantWithPolicy(self, policyType="SAV"):
        hostname = self.options.hostname
        return self.waitForCompliancy(hostname, policyType, True)

    def waitForNotCompliantWithPolicy(self, policyType="SAV"):
        hostname = self.options.hostname
        return self.waitForCompliancy(hostname, policyType, False)

    def getRegistrationCommand(self):
        # https://amzn-eu-west-1-f9b7.api-upe.d.hmr.sophos.com/frontend/api/deployment/agent/locations?transports=endpoint_mcs
        url = self.upe_api + "/deployment/agent/locations?transports=endpoint_mcs"
        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)
        response = json_loads(response)
        for platform in response:
            if platform['platform'] == "Linux" and platform['command'] != None:
                return platform['command']
        assert False, "Failed to find command for Linux registration"

    def getThinInstallerURL(self):
        # https://amzn-eu-west-1-f9b7.api-upe.d.hmr.sophos.com/frontend/api/deployment/agent/locations?transports=endpoint_mcs
        url = self.upe_api + "/deployment/agent/locations?transports=endpoint_mcs"
        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)
        response = json_loads(response)
        for platform in response:
            if platform['platform'] == "Linux" and platform['url'] != None:
                return platform['url']
        assert False, "Failed to find thin-installer URL"

    def getSSPLThinInstallerURL(self):
        url = self.upe_api + "/deployment/agent/location?platform=sspl&product_types=mdr"
        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)
        response = json_loads(response)
        if response['platform'] == "SSPL" and response['url'] != None:
            return response['url']
        assert False, "Failed to find sspl thin-installer URL"

    def setFlag(self, flag, value):
        url = self.host + "/api/customer/flags"

        value = {
            "1": True,
        }.get(value, value)

        data = json.dumps({'key': flag, "value": value}).encode('UTF-8')
        request = urllib.request.Request(url, data=data, headers=self.default_headers)
        response = self.retry_request_url(request)
        response = json_loads(response)
        return response

    def setFlags(self, *flags):
        res = {}
        for flag in flags:
            if '=' in flag:
                flag, value = flag.split('=', 1)
            else:
                value = True

            if res.get(flag) == value:
                print("flag %s already set to %s" % (flag, str(value)))
            else:
                res = self.setFlag(flag, value)
        return res

    def setUpdateCreds(self, username, password, address=""):
        if self.isConfiguredWithUpdateCache():
            raise AssertionError("""It is not safe to change credentials associated with update cache in tests.
         Current cloud options: {}.
         Aborting.""".format(self.options))

        url = self.upe_api + "/update_configuration/servers"
        data = json.dumps(
            {"use_default": False, "username": username, "password": password, "address": address}).encode('UTF-8')
        request = urllib.request.Request(url, data=data, headers=self.default_headers)
        request.get_method = lambda: 'PUT'
        response = self.retry_request_url(request)
        response = json_loads(response)
        return response

    def getUpdateUsername(self):
        url = self.upe_api + "/update_configuration/servers"
        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)
        response = json_loads(response)
        if response["use_default"]:
            return response["default_username"]
        else:
            return response["username"]

    def deletePolicy(self, policy):
        """
        https://api.linux.cloud.sandbox/api/policies/servers/547b186930045d1e6a9dee36
        """
        if isinstance(policy, str):
            policyid = policy
        else:
            policyid = policy['id']

        url = self.upe_api + "/policies/servers/" + policyid
        request = urllib.request.Request(url, headers=self.default_headers)
        request.get_method = lambda: 'DELETE'
        return self.retry_request_url(request)

    def clearPolicies(self):
        """
        Delete all policies other than "Base Policy"
        """
        policies = self.getServerPolicies()
        for policy in policies:
            print(str(policy), policy['id'])
            if policy['name'] != "Base Policy":
                self.deletePolicy(policy)

    def deleteInactiveServers(self):
        """
        Delete all servers inactive for more than 1 day
        """
        now = time.time()
        target = now - 24 * 60 * 60
        target = time.strftime("%Y-%m-%dT%H:%M:%S.000Z", time.gmtime(target))

        for server in self.getServers():
            if server['last_activity'] < target:
                print("DELETE", server['last_activity'], "<", target)
                self.deleteServerById(server['id'])
            else:
                print("KEEP  ", server['last_activity'], ">", target)

    def ensureEventsReady(self):
        """
        ## https://api.sandbox.sophos/api/events?endpoint=<endpointID>
        ## Since release/2018.37 of Central, getEvents cannot be called without first visiting the above URL.
        ## This occurs only when pulsar/nova is first deployed. Once you visit this URL, the getEvents request
        ## is fixed for every customer until the backend is deployed again.
        """
        hostname = self.options.hostname
        server = self.getServerByName(hostname)
        if server is None:
            print("Could not get endpoint id")
            return

        url = self.upe_api + "/events"

        query = {
            'endpoint': server['id'],
            'limit': 5,
        }
        url += "?" + urlencode(query, doseq=True)

        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)

    def getEvents(self, eventType, starttime, endtime):
        """
        ## https://api.linux.cloud.sandbox/api/reports/events?from=2014-09-10T00:00:00.000Z&limit=25
        ##  &offset=0&to=2014-12-09T00:00:00.000Z&types=Event::Endpoint::OutOfDate&
        ##  types=Event::Endpoint::UpdateSuccess&types=Event::Endpoint::UpdateFailure&
        ## types=Event::Endpoint::UpdateRebootRequired&types=Event::Endpoint::UpdateRebootUrgentlyRequired

        ## https://api.linux.cloud.sandbox/api/reports/events?
        ## from=2014-09-10T00:00:00.000Z&limit=25&offset=0&search=abelard&to=2014-12-09T00:00:00.000Z&types=Event::Endpoint::UpdateRebootRequired

        ## https://api.linux.cloud.sandbox/api/reports/events?
        ## from=2014-12-07T00:00:00.000Z&limit=25&offset=0&search=abelard&to=2014-12-09T00:00:00.000Z&types=Event::Endpoint::UpdateRebootRequired

        ## https://api.linux.cloud.sandbox/api/reports/events?
        ## search=abelard&from=2014-12-09T14:22:16.000Z&to=2014-12-09T14:38:56.000Z&limit=100&offset=0&types=Event::Endpoint::UpdateRebootRequired

        """

        # Call ensureEventsReady first to avoid NPEs in Pulsar and Nova
        self.ensureEventsReady()

        hostname = self.options.hostname
        starttime = int(starttime)
        endtime = float(endtime)

        types = []

        for t in eventType.split(";"):
            types.append({
                             "UpdateSuccess": "Event::Endpoint::UpdateSuccess",
                             "UpdateReboot": 'Event::Endpoint::UpdateRebootRequired',
                             "UpdateFailure": "Event::Endpoint::UpdateFailure",
                         }.get(t, t))

        target = time.strftime("%Y-%m-%dT%H:%M:%S.000Z", time.gmtime(starttime))
        end = time.strftime("%Y-%m-%dT%H:%M:%S.000Z", time.gmtime(endtime))

        url = self.upe_api + "/reports/events"

        # ~ url = url.replace("%3A",":")

        # ~ url = "https://api.linux.cloud.sandbox/api/reports/events?from=2014-12-09T15:00:00.000Z&limit=25&offset=0&search=abelard&to=2014-12-10T00:00:00.000Z&types=Event::Endpoint::UpdateRebootRequired"
        # ~ url = "https://api.linux.cloud.sandbox/api/reports/events?from=2014-12-09T15:00:00.000Z&limit=25&offset=0&search=abelard&types=Event::Endpoint::UpdateRebootRequired"

        query = {
            'from': target,
            'to': end,
            'limit': 25,
            'offset': 0,
            'types': types,
            'search': hostname,
            'includeTypes': 'true',
        }
        url += "?" + urlencode(query, doseq=True)

        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.retry_request_url(request)

        ## We get items outside the from/to fields so do our own filtering
        ## Rely on ISO dates being directly comparable as strings
        response = json_loads(response)
        items = response["items"]
        response["items"] = []
        for item in items:
            if item['type'] not in types:
                print("item type %s not expected (from %s)" % (item['type'], str(types)), file=sys.stderr)
            elif item['when'] >= target and item['when'] <= end:
                response["items"].append(item)
            else:
                # ~ print("Ignoring event outside target time %s < %s < %s"%(target,item['when'],end),file=sys.stderr)
                pass

        return response

    def getRebootEvents(self, starttime, endtime):
        return self.getEvents("UpdateReboot", starttime, endtime)

    def waitForEvent(self, eventType, starttime=None, timelimit=60):
        if starttime is None:
            starttime = time.time()

        timelimit = int(timelimit)
        end = time.time() + timelimit
        delay = 2
        attempts = 0

        def check():
            response = self.getEvents(eventType, starttime, end)
            events = response["items"]
            if len(events) > 0:
                return True
            return False

        while time.time() < end:
            if check():
                return 0
            time.sleep(delay)
            delay += 1

        return check()

    def waitForRebootEvent(self, starttime=None, timelimit=60):
        return self.waitForEvent("UpdateReboot", starttime, timelimit)

    def setFakePolicy(self, policyType, policyFile):
        assert self.__isFakeCloud()
        policy = open(policyFile).read()
        url = self.host + "/controller/%s/policy" % policyType
        request = urllib.request.Request(url, data=policy, headers=self.default_headers)
        request.get_method = lambda: "PUT"
        response = self.retry_request_url(request)
        return json_loads(response)

    #ruby uuid representation swaps nibbles in the java uuid
    def get_java_id_from_endpoint_id(self, endpoint_id):
        java_id = ""
        for part in endpoint_id.split("-"):
            java_id += ''.join([c[1] + c[0] for c in zip(part[::2], part[1::2])])
            java_id += "-"
        return java_id[:-1]

    def run_live_query(self, query_name, query_string,  hostname=None ):
        if hostname is None:
            hostname = self.options.hostname

        server = self.getServerByName(hostname)
        if server is None:
            return

        url = self.upe_api + '/v1/live-query/executions'
        query_json = {"name": query_name, "template": query_string}
        data = json.dumps({"endpointIds": [self.get_java_id_from_endpoint_id(server['id'])], "adHocQuery": query_json}, separators=(',', ': ')).encode('UTF-8')
        request = urllib.request.Request(url, data=data, headers=self.default_headers)
        request.get_method = lambda: "POST"
        response_obj = self.retry_request_url(request)

        return response_obj

    def wait_for_live_query_response(self, pending_query_response):
        response_obj = json.loads(pending_query_response)
        query_id = response_obj["id"]
        url = self.upe_api + '/v1/live-query/executions/{}/endpoints'.format(query_id)
        timeout = time.time() + response_obj["maxDurationInSeconds"]

        while time.time() < timeout:
            request = urllib.request.Request(url, headers=self.default_headers)
            response = self.retry_request_url(request)
            endpoints_doing_query = json.loads(response)
            if len(endpoints_doing_query["items"]) > 0:
                still_waiting = False
                for ep in endpoints_doing_query["items"]:
                    if ep["status"] != "finished":
                        still_waiting = True
                if not still_waiting:
                    logger.info("Query status: {}".format(ep["result"]))
                    return
            time.sleep(0.3)

        logger.warning("timed out running".format(query_id))

    def run_live_query_and_wait_for_response(self, query_name, query_string, hostname=None):
        pending_query_response = self.run_live_query(query_name, query_string, hostname)
        self.wait_for_live_query_response(pending_query_response)

    def run_multiple_live_queries_and_wait_for_response(self, queries, hostname=None):
        pending_query_responses = {}
        for name, query in queries.items():
            pending_query_responses[name] = self.run_live_query(name, query, hostname)

        for pending_response in pending_query_responses.values():
            self.wait_for_live_query_response(pending_response)

    #parser = cloudClient.add_options()
    #options, args = parser.parse_args(["-r", "q"])
    #options.email="darwinperformance@sophos.xmas.testqa.com"

def deleteServerFromCloud(options, args):
    hostname = host(args[1])
    client = CloudClient(options)
    output = client.deleteServerByName(hostname)
    print(output, file=sys.stderr)
    return 0


def scanNowFromCloud(options, args):
    hostname = host(args[1])
    client = CloudClient(options)
    client.scanNow(hostname)
    return 0


def checkServerInCloud(options, args):
    hostname = host(args[1])
    client = CloudClient(options)

    if client.getServerByName(hostname) is None:
        return 1
    return 0


def waitForServerInCloud(options, args):
    hostname = host(args[1])
    client = CloudClient(options)

    start = time.time()
    delay = 1
    while time.time() < start + options.wait:
        if client.getServerByName(hostname) is not None:
            return 0
        time.sleep(delay)
        delay += 1

    if client.getServerByName(hostname) is not None:
        return 0
    return 1


def getServerReport(options, args):
    hostname = host(args[1])
    client = CloudClient(options)

    server_report = client.getServerReportByName(hostname)

    if len(args) > 2:
        print(server_report[args[2]])
    else:
        print(server_report)


def checkServerReport(options, args):
    assert len(args) > 3

    hostname = host(args[1])
    key = args[2]
    expected = args[3]

    client = CloudClient(options)

    def check():
        server_report = client.getServerReportByName(hostname)
        if server_report is not None:
            print("Server report is [" + str(server_report) + "]")
            actual = str(server_report[key])
            if actual == expected:
                return 0
        return 1

    start = time.time()
    delay = 1
    while time.time() < start + options.wait:
        if check() == 0:
            return 0
        time.sleep(delay)
        delay += 1

    ## Final check after timer expired
    return check()


def cloneServerPolicy(options, args):
    assert len(args) == 3
    basename = args[1]
    newname = args[2]

    client = CloudClient(options)

    client.cloneServerPolicy(basename, newname)


def cloneBaseServerPolicy(options, args):
    return cloneServerPolicy(options, [args[0], "Base Policy", args[1]])


def genericMethod(options, args):
    command = args[0].replace(" ","")
    args = args[1:]
    options.wait = float(options.wait)
    client = CloudClient(options)
    methodToCall = getattr(client, command)
    ret = methodToCall(*args)
    if ret is None:
        return 0
    elif isinstance(ret, str):
        print(ret)
        return 0
    elif isinstance(ret, bool):
        if ret:
            return 0
        return 1
    elif isinstance(ret, int):
        print(ret)
        return ret
    else:
        ## Anything else json dump
        ret = json.dumps(ret, indent=2, sort_keys=True)
        print("Response: %s" % (ret))
        return 0


def downloadFile(url, path, certificate):

    conn = http.client.HTTPSConnection(
        url,
        cert_file=certificate
    )
    conn.putrequest('GET', path)
    conn.endheaders()
    response = conn.getresponse()
    print(response.read())


def processArguments(options, args):
    options.hostname = host(options.hostname)
    if len(args) > 0:
        command = args[0].replace(" ","")
    else:
        logger.error("No command given")
        return 1

    if options.email is None:
        options.email = input('Email:    ')

    if options.password is None:
        options.password = getpass.getpass("Password for %s:" % options.email)

    try:
        try:
            if command in globals():
                return globals()[command](options, args)
            else:
                return genericMethod(options, args)
        except Exception as e:
            exception_string = type(e).__name__ + ": " + str(e)
            print(exception_string, file=sys.stderr)

            traceback.print_exc()

            TMPROOT = os.environ.get("TMPROOT", "/tmp/PeanutIntegration")
            with open(os.path.join(TMPROOT, "cloud_exception.txt"), "w") as outfile:
                print(exception_string, file=outfile)

            raise
    except HostMissingException as e:
        if options.ignore_missing_host or options.ignore_errors:
            return 0
        else:
            return 14
    except ssl.SSLError as e:
        if options.ignore_errors:
            return 1
        return 79
    except urllib.error.HTTPError as e:
        if options.ignore_errors:
            return 1
        return 78
    except urllib.error.URLError as e:
        if options.ignore_errors:
            return 1
        return 77
    except Exception as e:
        if options.ignore_errors:
            return 1
        return 14


def add_options():
    parser = OptionParser(description='Cloud test client for SAV Linux automation')
    parser.add_option('-r', '--region', default='linux', action='store',
                      help="Region where account exists. Eg . p : production, q : QA , sb : sandbox.")
    parser.add_option('-e', '--email', default=None, action="store", help="Email address of the account")
    parser.add_option('-p', '--password', default=None, action="store", help="Password for the account")
    parser.add_option('-w', '--wait', default=30, action="store", type="int", help="Period to wait for a checked value")
    parser.add_option('--proxy', default=None, action="store", help="Proxy for accessing sandbox")
    parser.add_option('--proxy-username', default=None, action="store",
                      help="Username for Proxy for accessing sandbox/Central")
    parser.add_option('--proxy-password', default=None, action="store",
                      help="Password for Proxy for accessing sandbox/Central")
    parser.add_option("-n", "--hostname", "--name", default=None, action="store",
                      help="Hostname to perform the action on")
    parser.add_option("--wait-for-host", default=False, action="store_true",
                      dest="wait_for_host",
                      help="When checking policy compliance wait for the machine to appear before checking")
    parser.add_option("--with-on-access", default=False, action="store_true",
                      dest="with_on_access",
                      help="When checking policy compliance also wait for on-access enabled, and on-access enabled in policy")
    parser.add_option("--without-on-access", default=False, action="store_true",
                      dest="without_on_access",
                      help="When checking policy compliance also wait for on-access disabled, and on-access disabled in policy")
    parser.add_option("--ignore-missing-host", default=False, action="store_true",
                      dest="ignore_missing_host",
                      help="Ignore the host missing exceptions")
    parser.add_option("--ignore-errors", default=False, action="store_true", dest="ignore_errors",
                      help="Always exit with 0 or 1")
    return parser

def process(args):
    logging.getLogger().setLevel(logging.DEBUG)

    parser = add_options()

    options, args = parser.parse_args(args)
    return processArguments(options, args)


def main(argv):
    logging.basicConfig()
    return process(argv[1:])


if __name__ == '__main__':
    sys.exit(main(sys.argv))

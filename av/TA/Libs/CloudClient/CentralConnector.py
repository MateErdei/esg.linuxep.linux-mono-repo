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
import urllib.parse

urlencode = urllib.parse.urlencode

try:
    from robot.api import logger
    logger.warning = logger.warn
except ImportError:
    import logging
    logger = logging.getLogger(__name__)

string_type = str

try:
    from . import SophosHTTPSClient
    from . import SophosRobotSupport
except ImportError:
    import SophosHTTPSClient
    import SophosRobotSupport

wrap_ssl_socket = SophosHTTPSClient.wrap_ssl_socket
set_ssl_context = SophosHTTPSClient.set_ssl_context
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

GL_ENSURE_EVENTS_READY = False

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
        self.upe_api = None
        self.__m_region = region
        self.__m_url = _getUrl(region)
        self.__m_username = _getUsername(region)
        self.__m_password = _getPassword(region)
        self.default_headers = {
            "content-type": "application/json;chatset=UTF-8",
            "accept": "application/json"
        }
        self.__m_my_hostname = _get_my_hostname()

        # To match regression suite cloudClient.py
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
            with open(os.path.join(TMPROOT, "cloud_tokens.txt"), "r") as infile:
                session_data = json.load(infile)
            if session_data['host'] == self.__m_url and session_data['credential'] == credential:
                self.default_headers['X-Hammer-Token'] = session_data['hammer_token']
                self.default_headers['X-CSRF-Token'] = session_data['csrf_token']
                self.upe_api = session_data['upe_url']
                logger.debug("Using cached cloud login")
        except IOError:
            pass

    def __retry_request_url(self, request, send_and_receive_json=True):
        if self.upe_api is None:
            self.login(request)

        assert 'X-Hammer-Token' in self.default_headers
        assert 'X-CSRF-Token' in self.default_headers

        #~ print(request.header_items(), file=sys.stderr)

        assert request.has_header('X-hammer-token') ## urllib2 title cases the header
        assert request.has_header('X-csrf-token') ## urllib2 title cases the header
        assert self.upe_api is not None

        if send_and_receive_json:
            request.add_header("content-type", "application/json;chatset=UTF-8")
            request.add_header("accept", "application/json")

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
                        logger.info("403 from request - logging in")
                        self.login(request)
                    else:
                        logger.info("403 from request - retrying")
                elif e.code == 401:
                    logger.warning("401 from request - Cached session timed out - logging in")
                    self.login(request)
                elif e.code == 400:
                    logger.error("400 Error from request")
                    logger.error("Request headers: %s" % (repr(request.headers)))
                    logger.error("Response headers: %s" % (e.headers.as_string()))
                    raise
                elif e.code == 500:
                    logger.error("500 Error from request")
                    logger.error("Request headers: %s" % (repr(request.headers)))
                    logger.error("Response headers: %s" % (e.headers.as_string()))
                    raise
                else:
                    raise
            except urllib.error.URLError as e:
                reason = e.reason
                logger.error("urllib.error.URLError - retrying")
                if e.errno == errno.ECONNREFUSED:
                    delay += 5 ## Increase the delay to handle a connection refused
            except ssl.SSLError as e:
                reason = str(e)
                logger.error("ssl.SSLError - retrying")

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

        csrf_token = json_response.get("csrf", None)

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
            'credential': credential.decode("UTF-8"),
            'hammer_token': hammer_token,
            'csrf_token': csrf_token,
            'upe_url': self.upe_api,
        }
        logger.debug("session_data: {}".format(repr(session_data)))
        TMPROOT = self.__tmp_root()
        with open(os.path.join(TMPROOT, "cloud_tokens.txt"), "w") as outfile:
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

    def get_upe_api(self, *paths):
        if self.upe_api is None:
            self.login()
        assert self.upe_api is not None
        return self.upe_api + "".join(paths)

    def getRegistrationCommand(self):
        #https://amzn-eu-west-1-f9b7.api-upe.d.hmr.sophos.com/frontend/api/deployment/agent/locations?transports=endpoint_mcs
        url = self.get_upe_api("/deployment/agent/locations?transports=endpoint_mcs")
        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.__retry_request_url(request)
        response = json_loads(response)
        for platform in response:
            if platform['platform'] == "Linux" and platform['command'] is not None:
                return platform['command']
        raise Exception("Failed to find command for Linux registration")

    def get_token_and_url_for_registration(self):
        command = self.getRegistrationCommand()
        logger.info("Registration command: {}".format(command))
        parts = command.split(" ")
        assert len(parts) > 2
        token = parts[-2]
        logger.info("Token: {}".format(token))
        return token, parts[-1]

    def __getServerPolicies(self, limit=None, offset=None, q=None):
        url = self.get_upe_api() + "/v1/policies"
        query = {"target": "servers"}
        if limit is not None:
            query["limit"] = limit
        if offset is not None:
            query["offset"] = offset
        if q is not None:
            query["q"] = q
        if any(query):
            url += "?" + urllib.parse.urlencode(query)

        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.__retry_request_url(request)

        return json_loads(response)

    def __getItems(self, response):
        return json_loads(response)["items"]

    def __getServers(self, limit=None, offset=None, q=None):
        url = self.get_upe_api() + "/servers"
        query = {}
        if limit is not None:
            query["limit"] = limit
        if offset is not None:
            query["offset"] = offset
        if q is not None:
            query["q"] = q
        if any(query):
            url += "?" + urllib.parse.urlencode(query)

        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.__retry_request_url(request)

        return self.__getItems(response)

    def __deletePolicy(self, policy):
        """
        https://api.linux.cloud.sandbox/api/policies/servers/547b186930045d1e6a9dee36
        """
        if isinstance(policy, string_type):
            policyid = policy
        else:
            policyid = policy['id']

        url = self.get_upe_api() + "/policies/servers/" + policyid
        request = urllib.request.Request(url, headers=self.default_headers)
        request.get_method = lambda: 'DELETE'
        return self.__retry_request_url(request)

    def __deleteDisabledOrUnappliedPoliciesAndReturnGoodOne(self, policies):
        """
        Iterates through policies, deleting any that are disabled, or have no endpoints assigned to them.
        If only one remains then we return it.
        Otherwise we throw an exception
        """
        goodPolicy = None
        for policy in policies:
            if not policy['enabled'] or \
                    len(policy['applies_to']['endpoints']) == 0:
                self.__deletePolicy(policy)
                continue

            logger.debug("Policy:", policy['id'], policy['category'], policy['applies_to']['endpoints'], policy)

            if goodPolicy is None:
                goodPolicy = policy
            #~ else:
            #~ print("Policy1:",goodPolicy['id'],goodPolicy['enabled'],goodPolicy['applies_to']['endpoints'],goodPolicy,file=sys.stderr)
            #~ raise Exception("Multiple assigned, enabled policies of name %s found"%(policy['name']))

        return goodPolicy

    def __getServerPolicyByName(self, name, category="threat_protection"):
        policies = self.__getServerPolicies(q=name)
        logger.debug("Found %d policies: %s" %( len (policies), repr([ p['name'] for p in policies ])))
        if len(policies) > 1:
            policies = [ p for p in policies if p['name'] == name ]
        if len(policies) > 1:
            policies = [ p for p in policies if p.get("category", category) == category ]

        if len(policies) == 0:
            return None

        if len(policies) > 1:
            # Delete all policies which manage no computers
            return self.__deleteDisabledOrUnappliedPoliciesAndReturnGoodOne(policies)

        return policies[0]

    def __cloneServerPolicy(self, originalname, newname, category="threat_protection"):
        original = self.__getServerPolicyByName(originalname, category=category)
        assert original is not None

        url = self.get_upe_api() + "/v1/policies/" + original['id'] + "/clones"
        logger.debug("Clone url: {}".format(url))

        original['name'] = newname
        original.pop("customer_id", None)
        original.pop("last_modified", None)
        original['expiry']['expires_at'] = "1970-01-01T00:00:00.000Z"

        new_policy = {'name': newname}

        data = json.dumps(original).encode('UTF-8')  # Name is ignored by server
        request = urllib.request.Request(url, data=data, headers=self.default_headers)
        request.add_header("content-type", "application/json;chatset=UTF-8")
        request.add_header("accept", "application/json")
        logger.error(repr(request.headers))
        response = self.__retry_request_url(request)
        return json_loads(response)

    def __getServerId(self, hostname=None, expect_missing=False):
        hostname = _get_my_hostname(hostname)
        servers = self.__getServers(q=hostname)
        # Below line is required if one hostname is a subset of another
        # e.g. abelard, abelardRHEL67vm
        servers = [ s for s in servers if s['name'] == hostname ]

        if len(servers) == 0:
            serverList = []
            for s in self.__getServers():  # Check all servers just in case
                serverList.append(s['name'])
                if s['name'] == hostname:
                    return s['id']
            if not expect_missing:
                logger.error("No server found with name %s, Found: [%s]" % (hostname, str(serverList)))
            return None

        if len(servers) > 1:
            # dump all of the servers found
            logger.error(json.dumps(servers, indent=2, sort_keys=True))
            raise Exception("Multiple servers of name %s found: %d" % (hostname, len(servers)))

        serverId = servers[0]['id']
        return serverId

    def ensure_av_policy_exists(self, hostname=None, category="threat_protection"):
        hostname = _get_my_hostname(hostname)
        policyname = "av"+hostname
        policy = self.__getServerPolicyByName(policyname, category=category)
        if policy is None or policy['name'] != policyname:
            policy = self.__cloneServerPolicy("Base Policy", policyname, category=category)
            assert policy is not None

        return policy


    def __ensureServerPolicy(self, hostname=None, category="threat_protection", expect_missing=False):
        """
        Get the server policy, and ensure it exists
        """
        hostname = _get_my_hostname(hostname)
        endpointid = self.__getServerId(hostname, expect_missing=expect_missing)
        if endpointid is None:
            raise HostMissingException()

        policyname = "av"+hostname
        policy = self.ensure_av_policy_exists(hostname, category=category)
        assert policy is not None

        # rename and enable the new policy
        policy['name'] = policyname
        policy['enabled'] = True
        policy['base'] = False
        policy['applies_to']['endpoints'] = [endpointid]
        policy['applies_to']['groups'] = []
        policy['applies_to']['users'] = []
        policy['applies_to']['domains'] = []
        policy['category'] = category
        policy.pop("customer_id", None)
        policy.pop("last_modified", None)
        policy.setdefault('expiry', {})['expires_at'] = "1970-01-01T00:00:00.000Z"

        return policy

    def __setServerPolicy(self, policy):
        if 'id' in policy:
            method = "PUT"
            url = self.get_upe_api() + "/v1/policies/"+policy['id']
        else:
            method = "POST"
            url = self.get_upe_api() + "/v1/policies"

        data = json.dumps(policy).encode('UTF-8')
        logger.info("Applying policy: %s" % data)
        request = urllib.request.Request(url, data=data, headers=self.default_headers)
        request.add_header("content-type", "application/json;chatset=UTF-8")
        request.add_header("accept", "application/json")
        request.get_method = lambda: method
        response = self.__retry_request_url(request)
        return json_loads(response)

    def configure_exclusions(self, exclusions):
        hostname = _get_my_hostname()

        policy = self.__ensureServerPolicy(hostname)
        assert policy is not None

        path = "malware/scheduled/"
        policy['settings'][path + "exclusions_enabled"] = len(exclusions) > 0
        policy['settings'][path + "posix_exclusions"] = exclusions
        return self.__setServerPolicy(policy)

    def waitForServerInCloud(self, wait_time=60):
        hostname = _get_my_hostname()
        start = time.time()
        delay = 1
        while time.time() < start + wait_time:
            if self.__getServerId(hostname, expect_missing=True) is not None:
                return 0
            time.sleep(delay)
            delay += 1

        # Log errors
        if self.__getServerId(hostname) is not None:
            return 0
        return 1

    def assign_antivirus_product_to_endpoint_in_central(self, hostname=None):
        hostname = _get_my_hostname(hostname)
        endpointid = self.__getServerId(hostname)
        if endpointid is None:
            raise HostMissingException()

        # https://dzr-api-amzn-eu-west-1-f9b7.api-upe.d.hmr.sophos.com/api/product-assignment
        url = self.get_upe_api() + "/product-assignment"
        # {"assigned_product":"antivirus","assigned_ids":["d2bed399-e0b8-0482-1b93-bf361d7ac2ae"],"removed_ids":[]}
        contents = {
            "assigned_product": "antivirus",
            "assigned_ids": [endpointid],
            "removed_ids": []
        }

        method = "POST"
        data = json.dumps(contents).encode('UTF-8')
        logger.info("Applying policy: %s" % data)
        request = urllib.request.Request(url, data=data, headers=self.default_headers)
        request.get_method = lambda: method
        response = self.__retry_request_url(request)
        if len(response) == 0:
            return None
        return json_loads(response)

    def scanNow(self, hostname=None):
        hostname = _get_my_hostname(hostname)
        url = self.get_upe_api() + "/action/scan_now"
        endpointid = self.__getServerId(hostname)
        if endpointid is None:
            raise HostMissingException()
        data = json.dumps({"endpoint_id": endpointid }).encode('UTF-8')
        request = urllib.request.Request(url, data=data, headers=self.default_headers)
        response = self.__retry_request_url(request)
        return response

    def __ensureEventsReady(self):
        """
        ## https://api.sandbox.sophos/api/events?endpoint=<endpointID>
        ## Since release/2018.37 of Central, getEvents cannot be called without first visiting the above URL.
        ## This occurs only when pulsar/nova is first deployed. Once you visit this URL, the getEvents request
        ## is fixed for every customer until the backend is deployed again.
        """
        global GL_ENSURE_EVENTS_READY
        if GL_ENSURE_EVENTS_READY:
            return
        GL_ENSURE_EVENTS_READY = True
        hostname = _get_my_hostname()
        endpointid = self.__getServerId(hostname)
        if endpointid is None:
            print("Could not get endpoint id")
            return

        url = self.get_upe_api() + "/events"

        query = {
            'endpoint': endpointid,
            'limit': 1,
        }
        url += "?" + urlencode(query, doseq=True)

        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.__retry_request_url(request)

    def getEvents(self, eventType, starttime, endtime=None):
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
        endtime = endtime or time.time()

        # Call ensureEventsReady first to avoid NPEs in Pulsar and Nova
        self.__ensureEventsReady()

        hostname = _get_my_hostname()
        starttime = int(starttime)
        endtime = float(endtime)

        types = []

        if eventType is not None:
            for t in eventType.split(";"):
                types.append({
                                 "UpdateSuccess": "Event::Endpoint::UpdateSuccess",
                                 "UpdateReboot": 'Event::Endpoint::UpdateRebootRequired',
                                 "UpdateFailure": "Event::Endpoint::UpdateFailure",
                                 "ScanComplete": "Event::Endpoint::SavScanComplete",
                             }.get(t, t))

        target = time.strftime("%Y-%m-%dT%H:%M:%S.000Z", time.gmtime(starttime))
        logger.debug(f"Start/From time: {target}")
        end = time.strftime("%Y-%m-%dT%H:%M:%S.000Z", time.gmtime(endtime))

        url = self.get_upe_api() + "/reports/events"

        #~ url = url.replace("%3A",":")

        #~ url = "https://api.linux.cloud.sandbox/api/reports/events?from=2014-12-09T15:00:00.000Z&limit=25&offset=0&search=abelard&to=2014-12-10T00:00:00.000Z&types=Event::Endpoint::UpdateRebootRequired"
        #~ url = "https://api.linux.cloud.sandbox/api/reports/events?from=2014-12-09T15:00:00.000Z&limit=25&offset=0&search=abelard&types=Event::Endpoint::UpdateRebootRequired"

        query = {
            'from': target,
            'to': end,
            'limit': 25,
            'offset': 0,
            'search': hostname,
            'includeTypes': 'true',
        }
        if len(types) > 0:
            query['types'] = types

        url += "?" + urlencode(query, doseq=True)

        request = urllib.request.Request(url, headers=self.default_headers)
        response = self.__retry_request_url(request)

        ## We get items outside the from/to fields so do our own filtering
        ## Rely on ISO dates being directly comparable as strings
        response = json_loads(response)
        items = response["items"]
        response["items"] = []
        for item in items:
            if item['type'] not in types:
                logger.error("item type %s not expected (from %s)" % (item['type'], str(types)))
            elif target <= item['when'] <= end:
                response["items"].append(item)
            else:
                #~ print("Ignoring event outside target time %s < %s < %s"%(target,item['when'],end),file=sys.stderr)
                pass

        return response

    def generateAlerts(self, hostname=None, serverId=None, limit=None, batchLimit=100, quietLimit=None):
        """
        @param quietLimit - return after processing N items, but don't warn if there are more pending

        https://api.sandbox.sophos/api/alerts/endpoint/bbed18a9-4750-043e-d8a8-598da763f76c?limit=0&offset=0
        https://api.sandbox.sophos/api/alerts/endpoint/bbed18a9-4750-043e-d8a8-598da763f76c?endpointId=bbed18a9-4750-043e-d8a8-598da763f76c&limit=3&offset=0
        """
        hostname = _get_my_hostname(hostname)
        if hostname is not None and serverId is None:
            serverId = self.__getServerId(hostname)

        offset = 0
        numberProcessed = 0

        if quietLimit is not None:
            batchLimit = min(batchLimit, quietLimit+1)

        while True:
            if serverId:
                url = self.get_upe_api() + "/alerts/endpoint/%s?limit=%d&offset=%d" % (serverId, batchLimit, offset)
            else:
                url = self.get_upe_api() + "/alerts?limit=%d&offset=%d" % (batchLimit, offset)
            request = urllib.request.Request(url, headers=self.default_headers)
            response = self.__retry_request_url(request)
            response = json_loads(response)
            total = response['total']
            items = response["items"]

            if len(items) == 0:
                return

            logger.debug("Got items %d to %d of %d items"%(offset + 1, offset + len(items), total))

            for alert in items:
                if hostname is not None and alert['location'] != hostname:
                    if serverId is not None:
                        logger.error(json.dumps(alert, indent=2))
                        raise Exception("Incorrect hostname in alert: expected=%s, actual=%s"%(hostname,alert['location']))
                    else:
                        ## Can't expect to only have our alerts if we don't have a serverId
                        continue

                if serverId is not None and alert['data']['endpoint_id'] != serverId:
                    logger.error(json.dumps(alert, indent=2))
                    raise Exception("Incorrect serverId in alert: expected=%s, actual=%s"%(serverId,alert['data']['endpoint_id']))

                numberProcessed += 1
                yield alert

                if limit is not None and numberProcessed >= limit:
                    ## consider raising an exception here?
                    logger.info("WARNING: generateAlerts exceeded limit of %d items, fetched %d of %d items" % (limit, numberProcessed, total))
                    return

                if quietLimit is not None and numberProcessed >= quietLimit:
                    ## Quiet Limit reached
                    return

            offset += len(items)
            if offset >= total:
                return

    def clearAllAlerts(self, hostname=None):
        hostname = _get_my_hostname(hostname)
        serverId = self.__getServerId(hostname)
        logger.info("clearAllAlerts hostname=%s serverId=%s" % (hostname, serverId))
        url = self.get_upe_api() + "/alerts/actions/clear"
        for iteration in range(20):
            alerts = self.generateAlerts(serverId=serverId, hostname=hostname)
            alert_ids = [ a['id'] for a in alerts ]
            if len(alert_ids) == 0:
                return
            alert_ids = alert_ids[:100]
            logger.debug("Clearing %d alerts" % (len(alert_ids)))

            data = json.dumps({'ids': alert_ids}).encode('UTF-8')
            request = urllib.request.Request(url, data=data, headers=self.default_headers)
            response = self.__retry_request_url(request)

        raise Exception("clearAllAlerts failed to clear alerts")

#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
mcs_connection Module
"""

import base64
import datetime
import http.client
import logging
import os
# urllib.parse in python 3
import urllib.parse
import xml.dom.minidom
import xml.parsers.expat
import json

import mcsrouter.mcsclient.mcs_commands  # pylint: disable=no-name-in-module, import-error
import mcsrouter.mcsclient.mcs_exception
import mcsrouter.utils.path_manager as path_manager
import mcsrouter.utils.xml_helper
import mcsrouter.utils.write_json
from mcsrouter import ip_selection
from mcsrouter import proxy_authorization
from mcsrouter import sophos_https
from mcsrouter.utils.byte2utf8 import to_utf8
from . import datafeeds

LOGGER = logging.getLogger(__name__)
ENVELOPE_LOGGER = logging.getLogger("ENVELOPES")

class EnvelopeHandler:
    '''
    When log-level is info this helper filters envelope logs by logging request/response pairs for
    messages that a different from previously received
    '''
    def __init__(self):
        self._last_message = ""
        self._last_request = ""

    @staticmethod
    def _is_get(message):
        return message.startswith("GET ")

    @staticmethod
    def _trim_body_from_response_request(message):
        return message.split(":")[0].strip()

    def set_request(self, last_request):
        if "/responses/endpoint/" in last_request:
            loggable_last_request = self._trim_body_from_response_request(last_request)
        else:
            loggable_last_request = last_request

        if self._is_get(last_request):
            ENVELOPE_LOGGER.debug(loggable_last_request)
        else:
            ENVELOPE_LOGGER.info(loggable_last_request)

        self._last_request = last_request

    def _safe_to_log(self, message):
        return 'RESPONSE: {}'.format(message)

    def log_answer(self, message):
        message_to_log = self._safe_to_log(message)
        if ENVELOPE_LOGGER.getEffectiveLevel() != logging.DEBUG:
            if message != self._last_message:
                if self._is_get(self._last_request):
                    ENVELOPE_LOGGER.info(self._last_request)
                ENVELOPE_LOGGER.info(message_to_log)
        else:
            ENVELOPE_LOGGER.debug(message_to_log)

        self._last_message = message


GLOBAL_ENVELOPE_HANDLER = EnvelopeHandler()


class MCSHttpException(mcsrouter.mcsclient.mcs_exception.MCSNetworkException):
    """
    MCSHttpException
    """

    def __init__(self, error_code, headers, body):
        """
        __init__
        """
        super(
            MCSHttpException,
            self).__init__(
                "MCS HTTP Error: code=%d" %
                (error_code))
        self.m_http_error_code = error_code
        self.__m_headers = headers
        self.__m_body = body
        self.__m_current_path = None

    def error_code(self):
        """
        error_code
        """
        return self.m_http_error_code

    def headers(self):
        """
        headers
        """
        return self.__m_headers

    def body(self):
        """
        body
        """
        return self.__m_body


class MCSHttpGatewayTimeoutException(MCSHttpException):
    """
    504
    """
    pass


class MCSHttpServiceUnavailableException(MCSHttpException):
    """
    503
    """
    pass

class MCSHttpInternalServerErrorException(MCSHttpException):
    """
    500
    """
    pass


class MCSHttpUnauthorizedException(MCSHttpException):
    """
    MCSHttpUnauthorizedException
    """
    pass

class MCSHttpTooManyRequestsException(MCSHttpException):
    """
    MCSHttpTooManyRequestsException
    This was introduced in XDR for Central to indicate that the datafeed 'scheduled_query' needs to back off and
    purge all remaining datafeed result files: https://wiki.sophos.net/display/SophosCloud/EMP%3A+data-feed
    """
    pass


MCS_DEFAULT_RESPONSE_SIZE_MIB = 10


def create_user_agent(product_version, registration_token, product="Linux"):
    """
    create_user_agent
    """
    reg_token = "regToken"
    if registration_token in ["unknown", "", None]:
        reg_token = ""

    return "Sophos MCS Client/{} {} sessions {}".format(product_version, product, reg_token)


class MCSConnection:
    """
    MCSConnection class
    """
    # pylint: disable=too-many-instance-attributes

    def __init__(self, config, product_version="unknown", install_dir=".."):
        """
        __init__
        """

        self.__m_config = config
        self.__m_debug = False
        self.__m_connection = None
        self.__m_mcs_url = None
        self.__m_current_path = ""
        path_manager.INST = install_dir

        cafile = path_manager.root_ca_path()
        ca_file_env = os.environ.get("MCS_CA", None)
        if ca_file_env not in ("", None) and os.path.isfile(ca_file_env):
            if os.path.isfile(path_manager.ca_env_override_flag_path()):
                LOGGER.warning("Using {} as certificate CA".format(ca_file_env))
                cafile = ca_file_env
            else:
                LOGGER.warning(
                    "Cannot use {} as certificate CA without override flag: {}".format(
                        ca_file_env,
                        path_manager.ca_env_override_flag_path()))
        cafile = self.__m_config.get_default("CAFILE", cafile)
        if cafile is None or not os.path.isfile(cafile):
            raise mcsrouter.mcsclient.mcs_exception.MCSCACertificateException(
                "Unable to load CA certificates from '{}' as it isn't a file".format(cafile))

        self.__m_ca_file = cafile

        registration_token = self.__m_config.get_default("MCSToken", "unknown")
        self.set_user_agent(
            create_user_agent(
                product_version,
                registration_token))
        self.__m_cookies = {}

        if config.get_default("LOGLEVEL", "") == "DEBUG":
            self.set_debug()

        self.__m_current_proxy = None
        self.__m_policy_proxy = None
        self.__m_policy_proxy_credentials_obfuscated = None
        self.__m_obfuscation_cache = {}
        self.__m_policy_urls = None
        self.__m_use_direct = True
        self.__m_message_relays = []

        self.__m_proxy_authenticators = {}
        self.__m_last_seen_http_error = None

        self.m_jwt_token = None
        self.m_device_id = None
        self.m_tenant_id = None

    def ca_cert(self):
        return self.__m_ca_file

    def set_user_agent(self, agent):
        """
        set_user_agent
        """
        LOGGER.debug("Setting User-Agent to {}".format(agent))
        self.__m_user_agent = agent

    def __get_message_relays(self):
        """
        __get_message_relays
        """
        message_relay_list = []
        index = 1
        config = self.__m_config
        relay = config.get_default("message_relay_address%d" % index, False)
        while relay:
            relay_dict = {}
            relay_dict['hostname'] = relay
            relay_dict['port'] = config.get_default(
                "message_relay_port%d" % index)
            relay_dict['priority'] = config.get_default(
                "message_relay_priority%d" % index)
            relay_dict['id'] = config.get_default("message_relay_id%d" % index)
            message_relay_list.append(relay_dict)
            index += 1
            relay = config.get_default(
                "message_relay_address%d" %
                index, False)
        return message_relay_list

    def __message_relays_changed(self):
        """
        __message_relays_changed
        """
        current = self.__get_message_relays()

        if current:
            old_relays = []
            for relay in self.__m_message_relays:
                relay_string = relay["hostname"] + ":" + \
                    relay["port"] + ":" + relay["priority"]
                old_relays.append(relay_string)

            new_relays = []
            for relay in current:
                relay_string = relay["hostname"] + ":" + \
                    relay["port"] + ":" + relay["priority"]
                new_relays.append(relay_string)

            return set(new_relays) != set(old_relays)

        return len(current) != len(self.__m_message_relays)

    def __policy_proxy_changed(self):
        """
        __policy_proxy_changed
        """
        return (
            self.__m_policy_proxy != self.__m_config.get_default("mcs_policy_proxy", None)
            or self.__m_policy_proxy_credentials_obfuscated != self.__m_config.get_default(
                "mcs_policy_proxy_credentials",
                None))

    def __deobfuscate(self, obfuscated):
        """
        __deobfuscate
        """
        if obfuscated is None:
            return None

        if obfuscated not in self.__m_obfuscation_cache:
            from mcsrouter.utils import sec_obfuscation
            try:
                self.__m_obfuscation_cache[obfuscated] = sec_obfuscation.deobfuscate(
                    obfuscated)
            except sec_obfuscation.SECObfuscationException as exception:
                LOGGER.error(
                    "Invalid obfuscated credentials ({}): {}".format(str(exception), obfuscated))
                self.__m_obfuscation_cache[obfuscated] = None

        return self.__m_obfuscation_cache[obfuscated]

    def __get_deobfuscated_creds(self):
        self.__m_policy_proxy_credentials_obfuscated = self.__m_config.get_default(
            "mcs_policy_proxy_credentials", None)
        username = None
        password = None
        creds = self.__deobfuscate(
            self.__m_policy_proxy_credentials_obfuscated)
        if creds is not None and ":" in creds:
            username, password = creds.split(":", 1)
        return username, password

    def create_list_of_proxies_for_push_client(self):
        # setting the other return values here to member variables breaks MCSRouter proxy handling. So we don't do that.
        proxies, _, _, _ = self.__create_list_of_proxies()
        return proxies

    def __create_list_of_proxies_for_poll(self):
        proxies, message_relays, policy_proxy, use_direct = self.__create_list_of_proxies()
        self.__m_message_relays = message_relays
        self.__m_policy_proxy = policy_proxy
        self.__m_use_direct = use_direct
        return proxies

    def __create_list_of_proxies(self):
        """

        :return:
        Proxies: list of proxies to use
        message_relays: used to set a member variable of the MCSConnection object after returning
        policy_proxy: used to set a member variable of the MCSConnection object after returning
        use_direct: used to set a member variable of the MCSConnection object after returning
        """
        proxies = []
        config = self.__m_config
        username, password = self.__get_deobfuscated_creds()

        def add_proxy(
                proxy_string,
                relay_id=None,
                username=None,
                password=None):
            """
            add_proxy
            """
            if proxy_string is None:
                return

            proxy = sophos_https.Proxy(
                proxy_string,
                relay_id=relay_id,
                username=username,
                password=password)
            if proxy in proxies:
                return
            proxies.append(proxy)

        message_relays = self.__get_message_relays()
        ordered_message_relay_list = ip_selection.evaluate_address_preference(
            message_relays)
        for relay in ordered_message_relay_list:
            add_proxy(
                relay['hostname'] +
                ":" +
                relay['port'],
                relay['id'],
                username=username,
                password=password)

        policy_proxy = config.get_default("mcs_policy_proxy", None)
        add_proxy(policy_proxy, username=username, password=password)

        if config.get_bool("useSystemProxy", True):
            # Saved environment
            saved_proxy = config.get_default("proxy", None)
            add_proxy(saved_proxy)

            # Current environment
            env_proxy = os.environ.get("https_proxy", None)
            add_proxy(env_proxy)

        use_direct = self.__get_use_direct()
        if use_direct:
            # Try direct unless useDirect is set to false
            proxies.append(sophos_https.Proxy())

        return proxies, message_relays, policy_proxy, use_direct

    def get_id(self):
        """
        get_id
        """
        return self.__m_config.get_default('MCSID', None)

    def get_password(self):
        """
        get_password
        """
        return self.__m_config.get_default('MCSPassword', None)

    def __get_use_direct(self):
        """
        __get_use_direct
        """
        return self.__m_config.get_bool("useDirect", True)

    def __get_policy_urls(self):
        """
        __get_policy_urls
        """
        index = 1
        urls = []
        while True:
            field = "mcs_policy_url%d" % index
            val = self.__m_config.get_default(field, None)
            if val is None:
                break
            else:
                urls.append(val)
            index += 1
        return urls

    def __policy_urls_changed(self):
        """
        __policy_urls_changed
        """
        current = self.__get_policy_urls()
        return current != self.__m_policy_urls

    def __get_urls(self):
        """
        __get_urls
        """
        self.__m_policy_urls = self.__get_policy_urls()
        urls = self.__m_policy_urls[:]
        mcs_url = self.__m_config.get_default('MCSURL', None)
        if mcs_url is not None and mcs_url not in urls:
            urls.append(mcs_url)
        return urls

    def __get_url_parts(self, mcs_url):
        """
        @return (host,port,path)
        """

        mcs_url_parsed = urllib.parse.urlparse(mcs_url)
        (host, port) = sophos_https.split_host_port(mcs_url_parsed.netloc, 443)

        if self.__m_mcs_url != mcs_url:
            self.__m_mcs_url = mcs_url
            LOGGER.info("MCS URL {}:{}{}".format(host, port, mcs_url_parsed.path))

        return (host, port, mcs_url_parsed.path)

    def set_debug(self, debug=True):
        """
        set_debug
        """
        self.__m_debug = debug

    def get_authenticator_for_proxy(self, proxy, host, port):
        """
        get_authenticator_for_proxy
        """
        key = (proxy.host(),
               proxy.port(),
               proxy.username(),
               proxy.password(),
               host,
               port)
        auth = self.__m_proxy_authenticators.get(key, None)
        if auth is None:
            auth = proxy_authorization.ProxyAuthorization(proxy, host, port)
            self.__m_proxy_authenticators[key] = auth
        return auth

    def _try_create_connection(self, proxy, host, port):
        """
        __try_create_connection
        """
        # pylint: disable=too-many-branches, too-many-statements
        (proxy_host, proxy_port) = (proxy.host(), proxy.port())

        connection = None
        args = {"ca_certs": self.__m_ca_file}
        auth_calculator = self.get_authenticator_for_proxy(proxy, host, port)
        retry_count = 0
        retry = True
        while retry and retry_count < 5:
            retry = False
            retry_count += 1

            if proxy_host:
                if proxy.relay_id():
                    LOGGER.info("Trying connection via message relay {}:{}".format(proxy_host, proxy_port))
                else:
                    LOGGER.info("Trying connection via proxy {}:{}".format(proxy_host, proxy_port))
                connection = sophos_https.CertValidatingHTTPSConnection(
                    proxy_host, proxy_port, timeout=30, **args)
                proxy_username_password = auth_calculator.auth_header()
                proxy_headers = sophos_https.set_proxy_headers(proxy_username_password)
                connection.set_tunnel(host, port, headers=proxy_headers)
            else:
                LOGGER.info("Trying connection directly to {}:{}".format(host, port))
                connection = sophos_https.CertValidatingHTTPSConnection(host,
                                                                        port,
                                                                        timeout=30,
                                                                        **args)
            try:
                connection.connect()
            except sophos_https.ProxyTunnelError as exception:
                assert proxy_host is not None
                retry = auth_calculator.update_auth_header(exception.response)
                exception.close()
                if retry and retry_count < 5:
                    LOGGER.info(
                        "Retrying 407 response with updated auth header")
                else:
                    LOGGER.warning(
                        "Failed connection with proxy due to authentication via {}:{} to {}:{}: {}".format(
                            proxy_host,
                            proxy_port,
                            host,
                            port,
                            str(exception))
                    )
                    return None
            except mcsrouter.mcsclient.mcs_exception.MCSConnectionFailedException as exception:
                if proxy_host:
                    if proxy.relay_id():
                        LOGGER.warning(
                            "Failed connection with message relay via {}:{} to {}:{}: {} {}".format(
                                proxy_host,
                                proxy_port,
                                host,
                                port,
                                str(exception),
                                repr(exception))
                            )
                    else:
                        LOGGER.warning(
                            "Failed connection with proxy via {}:{} to {}:{}: {} {}".format(
                                proxy_host,
                                proxy_port,
                                host,
                                port,
                                str(exception),
                                repr(exception))
                            )
                else:
                    LOGGER.warning("Failed direct connection to {}:{} {}".format(
                        host, port, exception))
                return None

        # Success
        if proxy_host:
            LOGGER.info("Successfully connected to {}:{} via {}:{}".format(
                host, port, proxy_host, proxy_port))
        else:
            local_port = str(connection.sock.getsockname()[1])
            LOGGER.info(
                "Successfully directly connected to {}:{} from port {}".format(
                    host,
                    port,
                    local_port)
            )

        if self.__m_current_proxy != proxy:
            self.__m_current_proxy = proxy
            self.__m_config.set("current_relay_id", proxy.relay_id())
            mcsrouter.utils.write_json.write_current_proxy_info(proxy)
            self.__m_config.save()

        return connection

    def __get_response(self, request_data):
        """
        __get_response
        """
        # pylint: disable=too-many-branches, too-many-statements, too-many-locals
        path, headers, body, method = request_data
        if isinstance(body, str):
            encoded_body = body.encode('utf-8')
        else:
            encoded_body = body

        conn = self.__m_connection

        # currentPath is only defined once we have opened a connection
        base_path = self.__m_current_path
        full_path = base_path + path
        conn.request(method, full_path, body=encoded_body, headers=headers)
        response = conn.getresponse()
        response_headers = {
            key.lower(): value for key,
            value in response.getheaders()}

        length = response_headers.get('content-length', None)
        if length is None:
            limit = MCS_DEFAULT_RESPONSE_SIZE_MIB * 1024 * 1024
        else:
            limit = int(length)
            # Allow up to 10* our default limit if the content-length is
            # specified
            if limit > MCS_DEFAULT_RESPONSE_SIZE_MIB * 10 * 1024 * 1024:
                LOGGER.error("Content-Length too large")
                raise mcsrouter.mcsclient.mcs_exception.MCSNetworkException(
                    "Content-Length too large")

        # read one extra byte, if that succeeds we know the response is too
        # large
        body = response.read(limit + 1)
        if len(body) > limit:
            if length is None:
                LOGGER.error(
                    "Response too long, no content-length in headers, \
                     and more than %d MiB received (%d)",
                    MCS_DEFAULT_RESPONSE_SIZE_MIB,
                    len(body))
            else:
                LOGGER.error(
                    "Response too long, got %d, expected %d",
                    len(body),
                    limit)
            raise mcsrouter.mcsclient.mcs_exception.MCSNetworkException(
                "Response too long")

        # if we got an HTTP Content-Length, make sure the response isn't too
        # short
        if length is not None and len(body) < limit:
            LOGGER.error("Response too short. Size: {}. Limit: {}".format(len(body), limit))
            raise mcsrouter.mcsclient.mcs_exception.MCSNetworkException(
                "Response too short")

        if response.status == http.client.UNAUTHORIZED:
            LOGGER.info("UNAUTHORIZED from server {} WWW-Authenticate={}".format(
                response.status,
                response_headers.get('www-authenticate', "<Absent>")))
            LOGGER.debug("HEADERS={}".format(response_headers))
            raise MCSHttpUnauthorizedException(response.status, response_headers, body)
        if response.status == http.client.SERVICE_UNAVAILABLE:
            LOGGER.warning("HTTP Service Unavailable (503): {} ({})".format(
                response.reason, body))
            raise MCSHttpServiceUnavailableException(response.status, response_headers, body)
        if response.status == http.client.GATEWAY_TIMEOUT:
            LOGGER.warning("HTTP Gateway timeout (504): {} ({})".format(
                response.reason, body))
            raise MCSHttpGatewayTimeoutException(response.status, response_headers, body)
        if response.status == http.client.INTERNAL_SERVER_ERROR:
            LOGGER.warning("HTTP Internal Server Error (500): {} ({})".format(
                response.reason, body))
            raise MCSHttpInternalServerErrorException(response.status, response_headers, body)
        if response.status == http.client.TOO_MANY_REQUESTS:
            LOGGER.warning("HTTP Too Many Requests (429): {} ({})".format(response.reason, body))
            raise MCSHttpTooManyRequestsException(response.status, response_headers, body)
        if response.status != http.client.OK:
            LOGGER.error("Bad response from server {}: {} ({})".format(
                response.status, response.reason,
                http.client.responses.get(response.status,
                                          str(response.status))))
            raise MCSHttpException(response.status, response_headers, body)

        response_headers = response.getheaders()
        for header_key, header_value in response_headers:
            if header_key.lower() == "set-cookie":
                cookie = header_value.split(";", 1)[0].strip()
                name, value = cookie.split("=", 1)
                self.__m_cookies[name] = value
                LOGGER.debug("Storing cookie: {}={}".format(name, value))

        ENVELOPE_LOGGER.debug("response headers={}".format(str(response_headers)))

        if body:
            # body as a result of HTTPResponse.read return bytes.

            # Fix issue where we receive latin1 encoded characters in
            # XML received from Central (LINUXEP-4819)
            try:
                body_decoded = body.decode('utf-8')
            except UnicodeDecodeError:
                LOGGER.warning(
                    "Cannot decode response as UTF-8, treating as Latin1")
                # keep strings as utf-8 by default as this is the way we talk to central.
                body_decoded = body.decode('latin-1').encode('utf-8').decode('utf-8')

            body = body_decoded
            GLOBAL_ENVELOPE_HANDLER.log_answer(body)
        return (response_headers, body)

    def __try_get_response(self, request_data):
        """
        __try_get_response
        """
        try:
            return self.__get_response(request_data)

        except http.client.NotConnected as exception:
            # Only reported if it would otherwise have autoconnected
            self.__m_last_seen_http_error = exception
            LOGGER.info("Connection broken")
            self.close()
            return None

        except http.client.BadStatusLine as exception:
            self.__m_last_seen_http_error = exception
            LOGGER.info("Received httplib.BadStatusLine, closing connection")
            self.__m_cookies.clear()
            self.close()
            return None

        except MCSHttpException:
            # Immediately raise HTTP errors, since these are coming
            # directly from Central, so we shouldn't consider alternative routes
            raise

        # Failed to get a response from the server, or the connection failed.
        # Close the connection.
        except Exception as exception:  # pylint: disable=broad-except
            self.__m_last_seen_http_error = exception
            # don't re-use old cookies after an error, as this may trigger
            # de-duplication

            LOGGER.info("Forgetting cookies due to comms error: '{}'".format(str(exception)))
            self.__m_cookies.clear()
            self.__close_connection()
            return None

    def __try_url(self, mcs_url, proxies, request_data):
        """
        __try_url
        """
        previous_proxy = self.__m_current_proxy
        host, port, path = self.__get_url_parts(mcs_url)
        LOGGER.debug("Connecting to {}:{}{}".format(host, port, path))
        self.__m_current_path = ""

        def get_response_with_url(proxy):
            """
            get_response_with_url
            """
            self.__m_connection = self._try_create_connection(
                proxy, host, port)
            if self.__m_connection is not None:
                self.__m_current_path = path
                response = self.__try_get_response(request_data)
                return response
            return None

        if previous_proxy in proxies:
            # Re-try with the proxy that worked last time first, before we try
            # other proxies
            response = get_response_with_url(previous_proxy)
            if response:
                return response

        # Re-try different proxies - maybe the machine has moved
        self.__m_current_proxy = None
        for proxy in proxies:
            if proxy == previous_proxy:
                # Don't try the proxy we've already tried
                continue
            response = get_response_with_url(proxy)
            if response:
                return response

        assert self.__m_connection is None
        return None

    def __try_urls(self, request_data):
        """
        __try_urls
        """
        # Need to re-connect to Central/MCS
        proxies = self.__create_list_of_proxies_for_poll()
        urls = self.__get_urls()
        LOGGER.debug("Trying URLs: {}".format(str(urls)))

        # First try the URL that worked previously
        if self.__m_mcs_url in urls:
            response = self.__try_url(self.__m_mcs_url, proxies, request_data)
            if response:
                return response

        # Now try all other URLs
        for url in urls:
            if self.__m_mcs_url == url:
                # Don't try the previous URL if it didn't work
                continue
            response = self.__try_url(url, proxies, request_data)
            if response:
                return response

        # Failed to get a response in any fashion
        assert self.__m_connection is None

        # If we were unable to connect
        if not self.__m_last_seen_http_error:
            LOGGER.info("Failed to connect to Central")
            if not proxies:
                LOGGER.info(
                    "No proxies/message relays set to communicate \
                    with Central - useDirect is False")
            raise mcsrouter.mcsclient.mcs_exception.MCSConnectionFailedException(
                "Failed to connect to MCS")
        # If we were able to connect, but received a HTTP error
        # DELIBERATELY RE-RAISING PREVIOUS EXCEPTION!!!!!!
        # DO NOT CHANGE WITHOUT REVIEWING WITH DLCL
        assert self.__m_last_seen_http_error is not None
        raise self.__m_last_seen_http_error \
              or AssertionError("self.__m_last_seen_http_error is None")

    def __create_connection_and_get_response(self, request_data):
        """
        __create_connection_and_get_response
        """

        response = None
        self.__m_last_seen_http_error = None
        # If we have an existing connection
        if self.__m_connection is not None:
            if self.__policy_proxy_changed() or self.__message_relays_changed():
                # Need to close the current connection, because the policy has
                # changed
                LOGGER.info(
                    "Re-evaluating proxies / message relays due to changed policy")
                self.close()
                # Reset previous proxy - so that we re-evaluate
                self.__m_current_proxy = None

            if self.__policy_urls_changed():
                # Need to close the current connection, because the policy has
                # changed
                LOGGER.info("Re-evaluating MCS URL due to changed policy")
                self.close()
                self.__m_mcs_url = None

            if self.__get_use_direct() != self.__m_use_direct:
                LOGGER.info(
                    "Re-evaluating connection due to useDirect option change")
                self.__m_current_proxy = None
                self.close()

            # If we have an existing connection and the policy hasn't changed, do
            # the request with the existing connection.
            if self.__m_connection is not None:
                response = self.__try_get_response(request_data)

        # If we don't have a connection, or existing connection is broken
        # reevaluate URLs and Proxies.
        if self.__m_connection is None:
            response = self.__try_urls(request_data)

        assert self.__m_connection is not None
        assert response is not None
        return response

    def __close_connection(self):
        """
        __close_connection
        """
        if self.__m_connection is not None:
            self.__m_connection.close()
            self.__m_connection = None

    def close(self):
        """
        close
        """
        self.__close_connection()

    def _build_request_string(self, method, path, body):
        if body in (None, ""):
            return "{} {}".format(method, path)
        else:
            return "{} {} : {}".format(method, path, body)

    def __request(self, path, headers, body="", method="GET"):
        """
        __request
        """
        headers.setdefault("User-Agent", self.__m_user_agent)

        if self.__m_cookies:
            cookies = "; ".join(["=".join(cookie)
                                 for cookie in self.__m_cookies.items()])
            headers.setdefault("Cookie", cookies)
            LOGGER.debug("Sending cookies: {}".format(cookies))

        ENVELOPE_LOGGER.debug("request headers={}".format(str(headers)))
        request_string = self._build_request_string(method, path, body)
        GLOBAL_ENVELOPE_HANDLER.set_request(request_string)

        # Need to use the path from above, so that we can have different URLs
        request_data = (path, headers, body, method)
        return self.__create_connection_and_get_response(request_data)

    def capabilities(self):
        """
        capabilities
        """
        (headers, body) = self.__request("", {})  # pylint: disable=unused-variable
        return body

    def register(self, token, status_xml):
        """
        register
        """
        self.__m_cookies.clear()  # Reset cookies
        mcs_id = self.get_id() or ""
        if mcs_id == "reregister":
            mcs_id = ""
        password = self.get_password() or ""
        auth = "{}:{}:{}".format(mcs_id, password, token)
        auth = auth.encode('utf-8')
        headers = {
            "Authorization": "Basic {}".format(to_utf8(base64.b64encode(auth))),
            "Content-Type": "application/xml; charset=utf-8",
        }
        LOGGER.debug("Registering with auth    '{}'".format(auth))
        LOGGER.debug("Registering with message '{}'".format(status_xml))
        LOGGER.debug("Registering with headers '{}'".format(headers))
        (headers, body) = self.__request(
            "/register", headers, body=status_xml, method="POST")
        body = base64.b64decode(body)
        body = to_utf8(body)
        (endpoint_id, password) = body.split(":", 1)
        LOGGER.debug("Register returned endpoint_id '{}'".format(endpoint_id))
        LOGGER.debug("Register returned password   '{}'".format(password))
        return endpoint_id, password

    def action_completed(self, action):
        """
        action_completed
        """
        pass

    def _get_basic_authorization_header(self):
        bytes_to_be_encoded = "{}:{}".format(self.get_id(), self.get_password())
        bytes_to_be_encoded = bytes_to_be_encoded.encode("utf-8")
        return "Basic {}".format(to_utf8(base64.b64encode(bytes_to_be_encoded)))

    def send_message(self, command_path, body="", method="GET"):
        """
        send_message
        """
        headers = {
            "Authorization": self._get_basic_authorization_header(),
            "Content-Type": "application/xml; charset=utf-8",
        }
        if method != "GET":
            LOGGER.debug(
                "MCS request url={} body size={}".format(
                    command_path,
                    len(body)))
        (headers, body) = self.__request(command_path, headers, body, method)
        return body

    def send_message_with_id(self, command_path, body="", method="GET"):
        """
        send_message_with_id
        """
        return self.send_message(command_path + self.get_id(), body, method)

    def send_message_with_id_and_role(self, command_path, body="", method="GET"):
        """
        send_message_with_id_and_role
        """
        return self.send_message(command_path + self.get_id() + "/role/endpoint", body, method)

    def send_live_query_response_with_id(self, response):
        """
        prepare a HTTP request to send to central containing a LiveQuery response
        :param response: A response object (responses.py) which contains data from a livequery response file
        :return: The gzipped body of the LiveQuery response file
        """
        command_path = response.get_command_path(self.get_id())

        headers = {
            "Authorization": self._get_basic_authorization_header(),
            "Content-Type":  "application/json",
            "ActualSize": response.m_json_body_size
        }

        LOGGER.debug(
            "MCS request url={} body size={}".format(
                command_path,
                response.m_gzip_body_size))
        (headers, body) = self.__request(command_path, headers, response.m_json_body, "POST")
        return body

    def send_status_event(self, status):
        """
        send_status_event
        """
        try:
            status_xml = status.xml()
            self.send_message_with_id("/statuses/endpoint/", status_xml, "PUT")
        except mcsrouter.utils.xml_helper.XMLException:
            LOGGER.warning("Status xml rejected")

    def send_events(self, events):
        """
        send_events
        """
        try:
            events_xml = events.xml()
            self.send_message_with_id("/events/endpoint/", events_xml, "POST")
        except mcsrouter.utils.xml_helper.XMLException:
            LOGGER.warning("Event xml rejected")

    def send_responses(self, responses):
        """
        This method is used in mcs.py to trigger the sending of LiveQuery responses to central
        """
        def log_exception_error(app_id, correlation_id, exception):
            LOGGER.error("Failed to send response ({} : {}) : {}".format(app_id, correlation_id, exception))

        for response in responses:
            try:
                if response.m_json_body_size != 0:
                    self.send_live_query_response_with_id(response)
                else:
                    LOGGER.warning("Empty response (Correlation ID: {}). Not sending".format(response.m_correlation_id))
                response.remove_response_file()

            except (MCSHttpServiceUnavailableException, MCSHttpInternalServerErrorException) as exception:
                log_exception_error(response.m_app_id, response.m_correlation_id, exception)
                LOGGER.debug("Discarding response '{}' due to rejection by central".format(response.m_correlation_id))
                response.remove_response_file()

            except Exception as exception:
                log_exception_error(response.m_app_id, response.m_correlation_id, exception)

    def send_datafeeds(self, datafeeds: datafeeds.Datafeeds):
        """
        This method is used in mcs.py to trigger the processing and sending of datafeed results.
        """
        # Sending Protocol
        #   Discard files too large (e.g. 10MB)
        #   Discard files too old (e.g 2 weeks)
        #   Discard files oldest to newest to maintain a backlog less than limit (e.g. 1GB)
        #   Discard empty files
        #   Abide by back off
        #     If this is called during the back off time then return
        #   If we should purge (HTTP 429) then delete all files and return
        #   Sort files, sending oldest to newest
        #   Send files
        #     Only upload up to sending limit size (e.g. 10MB)
        #     Only upload up to maximum limit size (e.g. 10MB)
        #     Only upload up to a max frequency (e.g. every 60 seconds)

        LOGGER.debug(f"Pruning old datafeed files, datafeed ID: {datafeeds.get_feed_id()}")
        datafeeds.prune_old_datafeed_files()

        LOGGER.debug(f"Pruning datafeed files that are too large, datafeed ID: {datafeeds.get_feed_id()}")
        datafeeds.prune_too_large_datafeed_files()

        LOGGER.debug(f"Pruning backlog of datafeed files, datafeed ID: {datafeeds.get_feed_id()}")
        datafeeds.prune_backlog_to_max_size()

        LOGGER.debug(
            f"Sorting datafeed files oldest to newest ready for sending, datafeed ID: {datafeeds.get_feed_id()}")
        datafeeds.sort_oldest_to_newest()

        max_upload_at_once = datafeeds.get_max_upload_at_once()
        LOGGER.debug(
            f"Maximum single batch upload size is {max_upload_at_once} bytes, datafeed ID: {datafeeds.get_feed_id()}")

        ok_to_send_after = datafeeds.get_backoff_until_time()
        now = datetime.datetime.now().timestamp()
        if ok_to_send_after > now:
            return

        sent_so_far = 0
        for datafeed_result in datafeeds.get_datafeeds():
            if sent_so_far + datafeed_result.m_json_body_size > max_upload_at_once:
                LOGGER.debug("Can't send anymore datafeed results, at limit for now. Limit: {}".format(max_upload_at_once))
                break
            try:
                self.send_datafeed_result_v1(datafeed_result)
                LOGGER.info(f"Sent result, datafeed ID: {datafeeds.get_feed_id()}, file: {datafeed_result.m_file_path}")
                LOGGER.debug(
                    f"Result content for datafeed ID: {datafeeds.get_feed_id()}, content: {datafeed_result.m_json_body}")
                sent_so_far += datafeed_result.m_json_body_size
                datafeed_result.remove_datafeed_file()

            # Handle HTTP 429 (too many requests) from central
            except MCSHttpTooManyRequestsException as exception_429:

                # Handle purging
                purge = True

                try:
                    data_feed_response = json.loads(exception_429.body())
                    if data_feed_response:
                        purge = data_feed_response["purge"]
                except Exception:
                    LOGGER.error("Failed to parse response from datafeed when handling {} code".format(exception_429.error_code()))
                if purge:
                    LOGGER.warning("Purging all datafeed files due to 429 code from Sophos Central")
                    datafeeds.purge()

                # Handle Retry-After
                try:
                    retry_after = float(exception_429.headers().get("Retry-After", 60))
                    datafeeds.set_backoff_until_time(datetime.datetime.now().timestamp() + retry_after)
                except Exception:
                    LOGGER.warning("Failed read Retry-After in response headers from datafeed when handling {} code".format(exception_429.error_code()))

                break
            except Exception as exception:
                LOGGER.error("Failed to send datafeed: {}".format(exception))
                break

        # Clean up any files that were sent and no longer need to be in the datafeeds container.
        datafeeds.prune_results_with_no_files()

        # Set the next sending time, this may have already been set by the 429 exception handling above
        # so we make sure here we set it to the furthest in the future out of the two, i.e. backoff vs normal send freq.
        next_normal_send_at = datetime.datetime.now().timestamp() + datafeeds.get_max_send_freq()
        if next_normal_send_at > datafeeds.get_backoff_until_time():
            datafeeds.set_backoff_until_time(next_normal_send_at)

    def send_datafeed_result_v1(self, datafeed):
        """
        prepare a HTTP request to send to central containing a data feed result
        :param datafeed: A Datafeed object (datafeeds.py) which contains data, e.g. a scheduled query.
        :return: The compressed body of the datafeed file
        """
        command_path = datafeed.get_command_path(self.get_id())

        headers = {
            "Authorization": self._get_basic_authorization_header(),
            "Accept": "application/json",
            "Content-Length": datafeed.m_compressed_body_size,
            "Content-Encoding": "deflate",
            "X-Uncompressed-Content-Length": datafeed.m_json_body_size
        }

        LOGGER.debug(
            "MCS request url={} body size={}".format(
                command_path,
                datafeed.m_compressed_body_size))
        (headers, body) = self.__request(command_path, headers, datafeed.m_compressed_body, "POST")
        return body

    def extract_commands_from_xml(self, commands_xml):
        assert commands_xml is not None
        try:
            doc = mcsrouter.utils.xml_helper.parseString(commands_xml)
        except xml.parsers.expat.ExpatError as ex:
            LOGGER.error("Failed to parse commands: {}. Error: {}".format(xml, ex))
            return []
        try:
            command_nodes = doc.getElementsByTagName("command")
            commands = [
                # pylint: disable=no-member
                # pylint for some reason cannot see the mcs_commands is available.
                mcsrouter.mcsclient.mcs_commands.BasicCommand(
                    self,
                    node,
                    commands_xml) for node in command_nodes]
        except KeyError as key:
            LOGGER.error("Invalid command. Missing required field: {}".format(key))
            return []
        except ValueError as value_error:
            LOGGER.error("Invalid command: {}.".format(value_error))
            return []
        finally:
            doc.unlink()
        return commands

    def query_commands(self, app_ids=None):
        """
        Query any commands the server has for us
        """
        assert app_ids is not None

        # Get command XML from Central.
        commands = self.send_message_with_id("/commands/applications/{}/endpoint/".format(";".join(app_ids)))
        return self.extract_commands_from_xml(commands)

    def delete_command(self, command_id):
        """
        Delete a command that has been completed
        """
        self.send_message(
            "/commands/endpoint/{}/{}".format(self.get_id(), command_id),
            "", "DELETE")

    def get_policy(self, app_id, policy_id):
        """
        Get a policy from MCS
        """
        path = "/policy/application/{}/{}".format(app_id, policy_id)

        base_path = self.__m_current_path
        LOGGER.debug("Request policy from {}".format(base_path + path))
        return self.send_message(path)

    def get_flags(self):
        """
        Get flags from MCS
        """
        path = "/flags/endpoint/"
        base_path = self.__m_current_path
        LOGGER.debug("Request flags from {}".format(base_path + path))
        return self.send_message_with_id(path)

    def get_jwt_token(self):
        """
        Get JWT token from MCS
        """
        path = "/authenticate/endpoint/"
        base_path = self.__m_current_path
        LOGGER.debug("Request JWT token from {}".format(base_path + path))
        return self.send_message_with_id_and_role(path, method="POST")

    def get_policy_fragment(self, app_id, fragment_id):
        """
        get_policy_fragment
        """
        path = "/policy/fragment/application/{}/{}".format(app_id, fragment_id)

        base_path = self.__m_current_path
        LOGGER.debug("Request policy fragment from {}".format(base_path + path))
        return self.send_message(path)

    def set_jwt_token_settings(self):
        """
        set_jwt_token_settings
        """

        if not self.get_id():
            LOGGER.warning("No Endpoint ID so cannot retrieve JWT tokens")
            return

        full_token_response = self.get_jwt_token()
        try:
            token_dict = json.loads(full_token_response)
        except ValueError as error:
            LOGGER.error(f"Invalid JWT Token received: {full_token_response} with error: {error}")
            return

        if "access_token" in token_dict:
            LOGGER.debug(f"""Setting JWT Token: {token_dict["access_token"]}""")
            self.m_jwt_token = token_dict["access_token"]
        if "device_id" in token_dict:
            LOGGER.debug(f"""Setting Device ID: {token_dict["device_id"]}""")
            self.m_device_id = token_dict["device_id"]
        if "tenant_id" in token_dict:
            LOGGER.debug(f"""Setting Tenant ID: {token_dict["tenant_id"]}""")
            self.m_tenant_id = token_dict["tenant_id"]

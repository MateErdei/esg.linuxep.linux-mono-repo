#!/usr/bin/env python

import base64
import os
import xml.dom.minidom
import httplib


## urllib.parse in python 3
try:
    import urlparse
except ImportError:
    import urllib.parse as urlparse

import logging
logger = logging.getLogger(__name__)
envelopeLogger = logging.getLogger("ENVELOPES")

import MCSCommands
import MCSException

from mcsrouter import SophosHTTPS
from mcsrouter import IPSelection
from mcsrouter import ProxyAuthorization
import mcsrouter.utils.PathManager as PathManager


splitHostPort = SophosHTTPS.splitHostPort

class MCSHttpException(MCSException.MCSNetworkException):
    def __init__(self, errorCode, headers, body):
        super(MCSHttpException, self).__init__("MCS HTTP Error: code=%d"%(errorCode))
        self.m_httpErrorCode = errorCode
        self.__m_headers = headers
        self.__m_body = body
        self.__m_currentPath = None

    def errorCode(self):
        return self.m_httpErrorCode

    def headers(self):
        return self.__m_headers

    def body(self):
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

class MCSHttpUnauthorizedException(MCSHttpException):
    pass


MCS_DEFAULT_RESPONSE_SIZE_MIB = 10

def createUserAgent(productVersion, registrationToken, product="Linux"):
    regToken = "regToken"
    if registrationToken in ["unknown", "", None]:
        regToken = ""

    return "Sophos MCS Client/%s %s sessions %s"%(productVersion, product, regToken)

class MCSConnection(object):
    def __init__(self, config, productVersion="unknown",installDir=".."):

        self.__m_config = config
        self.__m_debug = False
        self.__m_connection = None
        self.__m_mcsurl = None
        self.__m_currentPath = ""
        PathManager.INST = installDir

        cafile = self.__m_config.getDefault("CAFILE",PathManager.rootCaPath())
        self.__m_cafile = None
        if cafile is not None and not os.path.isfile(cafile):
            logger.error("Unable to load CA certificates from %s as it isn't a file", cafile)
            cafile = None

        self.__m_cafile = cafile
        if cafile is None:
            logger.warning("Making unverified HTTPS connections")

        registrationToken = self.__m_config.getDefault("MCSToken","unknown")
        self.setUserAgent(createUserAgent(productVersion, registrationToken))
        self.__m_cookie = None

        if config.getDefault("LOGLEVEL","") == "DEBUG":
            self.setDebug()

        self.__m_currentProxy = None
        self.__m_policyProxy = None
        self.__m_policyProxyCredentialsObfuscated = None
        self.__m_obfuscationCache = {}
        self.__m_policyURLs = None
        self.__m_useDirect = True
        self.__m_messageRelays = []

        self.__m_proxyAuthenticators = {}
        self.__m_lastSeenHTTPError = None

    def setUserAgent(self, agent):
        logger.debug("Setting User-Agent to %s", agent)
        self.__m_userAgent = agent

    def __getMessageRelays(self):
        messageRelayList = []
        index = 1
        config = self.__m_config
        relay = config.getDefault("message_relay_address%d"%index, False)
        while relay:
            relayDict = {}
            relayDict['hostname'] = relay
            relayDict['port'] = config.getDefault("message_relay_port%d"%index)
            relayDict['priority'] = config.getDefault("message_relay_priority%d"%index)
            relayDict['id'] = config.getDefault("message_relay_id%d"%index)
            messageRelayList.append(relayDict)
            index += 1
            relay = config.getDefault("message_relay_address%d"%index, False)
        return messageRelayList

    def __messageRelaysChanged(self):
        current = self.__getMessageRelays()

        if len(current) > 0:
            oldRelays = []
            for relay in self.__m_messageRelays:
                relayString = relay["hostname"]+":"+relay["port"]+":"+relay["priority"]
                oldRelays.append(relayString)

            newRelays = []
            for relay in current:
                relayString = relay["hostname"]+":"+relay["port"]+":"+relay["priority"]
                newRelays.append(relayString)

            return set(newRelays) != set(oldRelays)

        return len(current) != len(self.__m_messageRelays)

    def __policyProxyChanged(self):
        return (
            self.__m_policyProxy != self.__m_config.getDefault("mcs_policy_proxy",None)
            or
            self.__m_policyProxyCredentialsObfuscated != self.__m_config.getDefault("mcs_policy_proxy_credentials",None)
            )

    def __deobfuscate(self, obfuscated):
        if obfuscated is None:
            return None

        if not self.__m_obfuscationCache.has_key(obfuscated):
            import SECObfuscation
            try:
                self.__m_obfuscationCache[obfuscated] = SECObfuscation.deobfuscate(obfuscated)
            except SECObfuscation.SECObfuscationException as e:
                logger.error("Invalid obfuscated credentials (%s): %s", str(e), obfuscated)
                self.__m_obfuscationCache[obfuscated] = None

        return self.__m_obfuscationCache[obfuscated]

    def __createListOfProxies(self):
        proxies = []
        config = self.__m_config
        self.__m_policyProxyCredentialsObfuscated = self.__m_config.getDefault("mcs_policy_proxy_credentials",None)
        username = None
        password = None
        creds = self.__deobfuscate(self.__m_policyProxyCredentialsObfuscated)
        if creds is not None and  ":" in creds:
            username,password = creds.split(":",1)

        def addProxy(proxyString, relayId=None, username=None, password=None):
            if proxyString is None:
                return

            proxy = SophosHTTPS.Proxy(proxyString, relayId=relayId, username=username,
                password=password)
            if proxy in proxies:
                return
            proxies.append(proxy)


        self.__m_messageRelays = self.__getMessageRelays()
        orderedMessageRelayList = IPSelection.evaluateAddressPreference(self.__m_messageRelays)
        for relay in orderedMessageRelayList:
            addProxy(relay['hostname'] + ":" + relay['port'], relay['id'],username=username,password=password)

        self.__m_policyProxy = config.getDefault("mcs_policy_proxy",None)
        addProxy(self.__m_policyProxy,username=username,password=password)

        if config.getBool("useSystemProxy",True):
            ## Saved environment
            saved_proxy = config.getDefault("proxy",None)
            addProxy(saved_proxy)

            ## Current environment
            env_proxy = os.environ.get("https_proxy", None)
            addProxy(env_proxy)

        self.__m_useDirect = self.__getUseDirect()
        if self.__m_useDirect:
            ## Try direct unless useDirect is set to false
            proxies.append(SophosHTTPS.Proxy())

        return proxies

    def getId(self):
        return self.__m_config.getDefault('MCSID',None)

    def getPassword(self):
        return self.__m_config.getDefault('MCSPassword',None)

    def __getUseDirect(self):
        return self.__m_config.getBool("useDirect",True)

    def __getPolicyURLs(self):
        index = 1
        urls = []
        while True:
            field = "mcs_policy_url%d"%index
            val = self.__m_config.getDefault(field,None)
            if val is None:
                break
            else:
                urls.append(val)
            index += 1
        return urls

    def __policyURLsChanged(self):
        current = self.__getPolicyURLs()
        return current != self.__m_policyURLs

    def __getURLs(self):
        self.__m_policyURLs = self.__getPolicyURLs()
        urls = self.__m_policyURLs[:]
        mcsurl = self.__m_config.getDefault('MCSURL',None)
        if mcsurl is not None and mcsurl not in urls:
            urls.append(mcsurl)
        return urls

    def __getURLParts(self, mcsurl):
        """
        @return (host,port,path)
        """

        mcsurl_parsed = urlparse.urlparse(mcsurl)
        (host, port) = splitHostPort(mcsurl_parsed.netloc, 443)

        if self.__m_mcsurl != mcsurl:
            self.__m_mcsurl = mcsurl
            logger.info("MCS URL %s:%d%s", host, port, mcsurl_parsed.path)

        return (host,port,mcsurl_parsed.path)

    def setDebug(self, debug=True):
        self.__m_debug = debug

    def getAuthenticatorForProxy(self, proxy, host, port):
        key = (proxy.host(),
               proxy.port(),
               proxy.username(),
               proxy.password(),
               host,
               port)
        auth = self.__m_proxyAuthenticators.get(key,None)
        if auth is None:
            auth = ProxyAuthorization.ProxyAuthorization(proxy, host, port)
            self.__m_proxyAuthenticators[key] = auth
        return auth

    def __try_createConnection(self, proxy, host, port):
        (proxyHost, proxyPort) = (proxy.host(),proxy.port())

        connection = None
        args = {"ca_certs": self.__m_cafile}
        ConnectionClass = SophosHTTPS.CertValidatingHTTPSConnection
        authCalculator = self.getAuthenticatorForProxy(proxy, host, port)
        retryCount = 0
        retry = True

        while retry and retryCount < 5:
            retry = False
            retryCount += 1

            if proxyHost:
                if proxy.relayId():
                    logger.info("Trying connection via message relay %s:%d",
                                proxyHost, proxyPort)
                else:
                    logger.info("Trying connection via proxy %s:%d",
                                proxyHost, proxyPort)
                connection = ConnectionClass(proxyHost, proxyPort, timeout=30, **args)
                proxyUsernamePassword = authCalculator.authHeader()
                if proxyUsernamePassword is None:
                    proxyHeaders = None
                else:
                    proxyHeaders = {
                        'Proxy-authorization' : proxyUsernamePassword
                    }
                connection.set_tunnel(host,port,headers=proxyHeaders)
            else:
                logger.info("Trying connection directly to %s:%d",
                            host, port)
                connection = ConnectionClass(host, port, timeout=30, **args)

            if self.__m_debug:
                #~ connection.set_debuglevel(1)
                pass

            try:
                connection.connect()
            except SophosHTTPS.ProxyTunnelError as e:
                assert proxyHost is not None
                retry = authCalculator.updateAuthHeader(e.response)
                e.close()
                if retry:
                    logger.info("Retrying 407 response with updated auth header")
                else:
                    logger.warning("Failed connection with proxy due to authentication via %s:%d to %s:%d: %s",
                        proxyHost, proxyPort,
                        host, port,
                        str(e),
                        )
                    return None
            except Exception as e:
                if proxyHost:
                    if proxy.relayId():
                        logger.warning("Failed connection with message relay via %s:%d to %s:%d: %s %s",
                                    proxyHost, proxyPort,
                                    host, port,
                                    str(e), repr(e))
                    else:
                        logger.warning("Failed connection with proxy via %s:%d to %s:%d: %s %s",
                                    proxyHost, proxyPort,
                                    host, port,
                                    str(e), repr(e))
                else:
                    logger.warning("Failed direct connection to %s:%d: %s %s",
                                host, port,
                                str(e), repr(e))
                return None

        ## Success
        if proxyHost:
            logger.info("Successfully connected to %s:%d via %s:%d",
                        host, port,
                        proxyHost, proxyPort)
        else:
            localport = str(connection.sock.getsockname()[1])
            logger.info("Successfully directly connected to %s:%d from port %s",
                        host, port, localport)

        if self.__m_currentProxy != proxy:
            self.__m_currentProxy = proxy
            self.__m_config.set("current_relay_id", proxy.relayId())
            self.__m_config.save()

        return connection

    def __getResponse(self, requestData):
        path, headers, body, method = requestData

        conn = self.__m_connection

        ## currentPath is only defined once we have opened a connection
        pathbase = self.__m_currentPath
        fullpath = pathbase + path

        conn.request(method, fullpath, body=body, headers=headers)
        response = conn.getresponse()
        response_headers = {k.lower(): v for k, v in response.getheaders()}

        length = response_headers.get('content-length', None)
        if length is None:
            limit = MCS_DEFAULT_RESPONSE_SIZE_MIB * 1024 * 1024
        else:
            limit = int(length)
            ## Allow up to 10* our default limit if the content-length is specified
            if limit > MCS_DEFAULT_RESPONSE_SIZE_MIB * 10 * 1024 * 1024:
                logger.error("Content-Length too large")
                raise MCSException.MCSNetworkException("Content-Length too large")

        # read one extra byte, if that succeeds we know the response is too large
        body = response.read(limit + 1)
        if len(body) > limit:
            if length is None:
                logger.error("Response too long, no content-length in headers, and more than %d MiB received (%d)",MCS_DEFAULT_RESPONSE_SIZE_MIB,len(body))
            else:
                logger.error("Response too long, got %d, expected %d",len(body),limit)
            raise MCSException.MCSNetworkException("Response too long")

        # if we got an HTTP Content-Length, make sure the response isn't too short
        if length is not None and len(body) < limit:
            logger.error("Response too short")
            raise MCSException.MCSNetworkException("Response too short")

        if response.status == httplib.UNAUTHORIZED:
            logger.info("UNAUTHORIZED from server %d WWW-Authenticate=%s",
                        response.status,
                        response_headers.get('www-authenticate', "<Absent>"))
            logger.debug("HEADERS=%s", str(response_headers))
            raise MCSHttpUnauthorizedException(response.status, response_headers, body)
        elif response.status == httplib.SERVICE_UNAVAILABLE:
            logger.warning("HTTP Service Unavailable (503): %s (%s)",
                        response.reason,
                        body)
            raise MCSHttpServiceUnavailableException(response.status, response_headers, body)
        elif response.status == httplib.GATEWAY_TIMEOUT:
            logger.warning("HTTP Gateway timeout (504): %s (%s)",
                        response.reason,
                        body)
            raise MCSHttpGatewayTimeoutException(response.status, response_headers, body)
        elif response.status != httplib.OK:
            logger.error("Bad response from server %d: %s (%s)",
                         response.status,
                         response.reason,
                         httplib.responses.get(response.status, str(response.status)))
            raise MCSHttpException(response.status, response_headers, body)

        # we should theoretically store cookies from HTTP errors responses too
        # but we *always* reset the connection for a non-HTTP-200 response.
        if "set-cookie" in response_headers:
            self.__m_cookie = response_headers["set-cookie"].split(";")[0]
            #~ logger.debug("Setting cookie to %s", self.__m_cookie)

        if body not in ("", None):
            envelopeLogger.info("RESPONSE: %s", body)

            # Fix issue where we receive latin1 encoded characters in
            # XML received from Central (LINUXEP-4819)
            try:
                body = body.decode("utf-8")
            except UnicodeDecodeError:
                logger.warning("Cannot decode response as UTF-8, treating as Latin1")
                body = body.decode("latin1")

        return (response_headers, body)

    def __tryGetResponse(self, requestData):
        try:
            return self.__getResponse(requestData)

        except httplib.NotConnected as e:
            ## Only reported if it would otherwise have autoconnected
            self.__m_lastSeenHTTPError = e
            logger.info("Connection broken")
            self.close()
            return

        except httplib.BadStatusLine as e:
            self.__m_lastSeenHTTPError = e
            logger.debug("Received httplib.BadStatusLine, closing connection")
            self.close()
            return

        # Failed to get a response from the server, or the connection failed. Close the connection.
        except Exception as e:
            self.__m_lastSeenHTTPError = e
            # don't re-use old cookies after an error, as this may trigger de-duplication
            logger.debug("Forgetting cookie due to comms error")
            self.__m_cookie = None
            self.__closeConnection()
            return

    def __tryURL(self, mcsurl, proxies, requestData):
        previousProxy = self.__m_currentProxy
        host,port,path = self.__getURLParts(mcsurl)
        logger.debug("Connecting to %s:%d%s",host,port,path)
        self.__m_currentPath = ""

        def getResponseWithURL(proxy):
            self.__m_connection = self.__try_createConnection(proxy, host, port)
            if self.__m_connection is not None:
                self.__m_currentPath = path
                response = self.__tryGetResponse(requestData)
                return response
            else:
                return

        if previousProxy in proxies:
            ## Re-try with the proxy that worked last time first, before we try other proxies
            response = getResponseWithURL(previousProxy)
            if response:
                return response

        ## Re-try different proxies - maybe the machine has moved
        self.__m_currentProxy = None
        for proxy in proxies:
            if proxy == previousProxy:
                ## Don't try the proxy we've already tried
                continue
            response = getResponseWithURL(proxy)
            if response:
                return response

        assert self.__m_connection is None
        return

    def __tryURLs(self, requestData):
        ## Need to re-connect to Central/MCS
        proxies = self.__createListOfProxies()
        urls = self.__getURLs()

        logger.debug("Trying URLs: %s",str(urls))

        ## First try the URL that worked previously
        if self.__m_mcsurl in urls:
            response = self.__tryURL(self.__m_mcsurl, proxies, requestData)
            if response:
                return response

        ## Now try all other URLs
        for url in urls:
            if self.__m_mcsurl == url:
                ## Don't try the previous URL if it didn't work
                continue
            response = self.__tryURL(url, proxies, requestData)
            if response:
                return response

        ## Failed to get a response in any fashion
        assert self.__m_connection is None

        ## If we were unable to connect
        if not self.__m_lastSeenHTTPError:
            logger.info("Failed to connect to Central")
            if len(proxies) == 0:
                logger.info("No proxies/message relays set to communicate with Central - useDirect is False")
            raise MCSException.MCSConnectionFailedException("Failed to connect to MCS")

        ## If we were able to connect, but received a HTTP error
        raise self.__m_lastSeenHTTPError

    def __createConnectionAndGetResponse(self, requestData):
        response = None
        self.__m_lastSeenHTTPError = None
        # If we have an existing connection
        if self.__m_connection is not None:
            if self.__policyProxyChanged() or self.__messageRelaysChanged():
                ## Need to close the current connection, because the policy has changed
                logger.info("Re-evaluating proxies / message relays due to changed policy")
                self.close()
                ## Reset previous proxy - so that we re-evaluate
                self.__m_currentProxy = None

            if self.__policyURLsChanged():
                ## Need to close the current connection, because the policy has changed
                logger.info("Re-evaluating MCS URL due to changed policy")
                self.close()
                self.__m_mcsurl = None

            if self.__getUseDirect() != self.__m_useDirect:
                logger.info("Re-evaluating connection due to useDirect option change")
                self.__m_currentProxy = None
                self.close()

            # If we have an existing connection and the policy hasn't changed, do
            # the request with the existing connection.
            if self.__m_connection is not None:
                response = self.__tryGetResponse(requestData)

        # If we don't have a connection, or existing connection is broken
        # reevaluate URLs and Proxies.
        if self.__m_connection is None:
            response = self.__tryURLs(requestData)

        assert self.__m_connection is not None
        assert response is not None
        return response

    def __closeConnection(self):
        if self.__m_connection is not None:
            self.__m_connection.close()
            self.__m_connection = None

    def close(self):
        self.__closeConnection()

    def __request(self, path, headers, body="", method="GET"):
        #~ logger.debug("%sing %s with %d byte body"%(method, path, len(body)))
        headers.setdefault("User-Agent", self.__m_userAgent)
        if self.__m_cookie is not None:
            headers.setdefault("Cookie", self.__m_cookie)
            #~ logger.debug("Sending cookie: %s", self.__m_cookie)

        if isinstance(body, unicode):
            body = body.encode("utf-8")

        if body in (None, ""):
            envelopeLogger.info("%s %s", method, path)
        else:
            envelopeLogger.info("%s %s : %s", method, path, body)

        ## Need to use the path from above, so that we can have different URLs
        requestData = (path, headers, body, method)
        return self.__createConnectionAndGetResponse(requestData)

    def capabilities(self):
        (headers, body) = self.__request("", {})
        return body

    def register(self, token, statusxml):
        self.__m_cookie = None ## Reset cookie
        eid = self.getId() or ""
        if eid == "reregister":
            eid = ""
        password = self.getPassword() or ""
        auth = "%s:%s:%s"%(eid, password, token)
        headers = {
            "Authorization":"Basic %s"%(base64.b64encode(auth)),
            "Content-Type":"application/xml; charset=utf-8",
            }
        logger.debug("Registering with auth    '%s'", auth)
        logger.debug("Registering with message '%s'", statusxml)
        logger.debug("Registering with headers '%s'", str(headers))
        (headers, body) = self.__request("/register", headers, body=statusxml, method="POST")
        body = base64.b64decode(body)
        (endpointid, password) = body.split(":", 1)
        logger.debug("Register returned endpointid '%s'", endpointid)
        logger.debug("Register returned password   '%s'", password)
        return (endpointid, password)

    def actionCompleted(self, action):
        pass

    def sendMessage(self, commandPath, body="", method="GET"):
        headers = {
            "Authorization":"Basic "+base64.b64encode("%s:%s"%(self.getId(),self.getPassword())),
            "Content-Type":"application/xml; charset=utf-8",
            }
        if method != "GET":
            logger.debug("MCS request url=%s body size=%d", commandPath, len(body))
        (headers, body) = self.__request(commandPath, headers, body, method)
        return body

    def sendMessageWithId(self, commandPath, body="", method="GET"):
        return self.sendMessage(commandPath+self.getId(), body, method)

    def sendStatusEvent(self, status):
        status_xml = status.xml()
        self.sendMessageWithId("/statuses/endpoint/", status_xml, "PUT")

    def sendEvents(self, events):
        events_xml = events.xml()
        self.sendMessageWithId("/events/endpoint/", events_xml, "POST")

    def queryCommands(self, appids=None):
        """
        Query any commands the server has for us
        """
        assert(appids is not None)
        commands = self.sendMessageWithId("/commands/applications/%s/endpoint/"%(";".join(appids)))

        try:
            doc = xml.dom.minidom.parseString(commands)
        except Exception:
            logger.exception("Failed to parse commands: %s", commands)
            return []

        try:
            commandNodes = doc.getElementsByTagName("command")
            commands = [MCSCommands.BasicCommand(self, node, commands) for node in commandNodes]
        finally:
            doc.unlink()

        return commands

    def deleteCommand(self, commandid):
        """
        Delete a command that has been completed
        """
        self.sendMessage("/commands/endpoint/%s/%s"%(self.getId(), commandid), "", "DELETE")

    def getPolicy(self, appid, policyid):
        """
        Get a policy from MCS
        """
        path = "/policy/application/%s/%s"%(appid, policyid)

        pathbase = self.__m_currentPath
        logger.debug("Request policy from %s", pathbase + path)
        return self.sendMessage(path)

    def getPolicyFragment(self, appid, fragmentid):
        path = "/policy/fragment/application/%s/%s"%(appid, fragmentid)

        pathbase = self.__m_currentPath
        logger.debug("Request policy fragment from %s", pathbase + path)
        return self.sendMessage(path)

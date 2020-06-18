#!/usr/bin/env python


## Copied from //dev/savlinux/regression/supportFiles/CloudAutomation/

import sys

import os
import re
import random
import base64
import json
import argparse
import ssl
import socket
import http.server
import subprocess
import time
import datetime
import xml.dom.minidom
import xml.sax.saxutils

import urllib.parse

import logging
import logging.handlers

current_dir = os.path.dirname(os.path.abspath(__file__))
libs_dir = os.path.abspath(os.path.join(current_dir, os.pardir, os.pardir, 'libs'))
sys.path.insert(1, libs_dir)

import ArgParserUtils

logger = logging.getLogger("cloudServer")
action_log = logging.getLogger("action")

socket.setdefaulttimeout(60)

ORIGINAL_PPID = os.getppid()


def _get_log_dir():
    return os.environ.get("LOGDIR", "tmp")


try:
    from builtins import str
except ImportError:
    pass

def getCloudAutomationDirectory():
    return os.path.dirname(os.path.abspath(__file__))
    ## os.environ.get('SUP', "supportFiles"), "CloudAutomation"

def setupLogging():
    rootLogger = logging.getLogger()
    rootLogger.setLevel(logging.DEBUG)

    formatter = logging.Formatter("%(asctime)s %(levelname)s %(name)s: %(message)s")

    streamHandler = logging.StreamHandler()
    streamHandler.setFormatter(formatter)
    streamHandler.setLevel(logging.DEBUG)
    rootLogger.addHandler(streamHandler)

    LOGDIR = _get_log_dir()
    try:
        os.makedirs(LOGDIR)
    except EnvironmentError:
        pass

    logfile = os.path.join(LOGDIR, "cloudServer.log")
    rotatingFileHandler = logging.handlers.RotatingFileHandler(logfile, maxBytes=1024*1024, backupCount=5)
    rotatingFileHandler.setFormatter(formatter)
    rotatingFileHandler.setLevel(logging.DEBUG)
    rootLogger.addHandler(rotatingFileHandler)

    logfile = os.path.join(LOGDIR, "cloudServer-action.log")
    fileHandler = logging.FileHandler(logfile, mode="w", encoding="UTF-8")
    fileHandler.setFormatter(
        logging.Formatter("%(message)s")
        )
    fileHandler.setLevel(logging.DEBUG)
    action_log.addHandler(fileHandler)

def getText(node):
    text = []
    for n in node.childNodes:
        if n.nodeType == xml.dom.Node.TEXT_NODE:
            text.append(n.data)
    return "".join(text)

ID = 1001
TESTTMP = os.environ.get("TESTTMP", "tmp")
TEST_ROOT = os.environ.get("TEST_ROOT", ".")
os.environ['OPENSSL_CONF'] = '/dev/null'

def openssl():
    p = os.path.join(TEST_ROOT, "bin", "openssl")
    if os.path.isfile(p):
        return p
    else:
        return "openssl"

class Command(object):
    def __init__(self):
        self.m_id = "FOOBAR"

    def createNode(self, doc, name, text):
        node = doc.createElement(name)
        textNode = doc.createTextNode(text)
        node.appendChild(textNode)
        return node

    def addToXml(self, doc):
        commands = doc.getElementsByTagName("commands")[0]
        command = doc.createElement("command")
        commands.appendChild(command)
        idnode = self.createNode(doc, "id", self.m_id)

class Policies(object):
    """<?xml version='1.0' encoding='UTF-8'?>
<policy RevID="d0b304d33f7d53560abd7c97d4d4158a15c02e34316566558cde181a2b8489ce">
    <!-- Destination where the endpoint should send heartbeat messages -->
    <destination address="127.0.0.1" port="4444"/>

    <enabled>true</enabled>
    <!-- Endpoint certificate renewal parameters -->
    <renewalparams triggerDaysBefore="90" switchDaysAfter="4" />

    <!-- The list of certificates stored for this endpoint, in PEM format -->
    <epcert fingerprint="...">
        -----BEGIN CERTIFICATE-----
        MIIEFzCCAv+gAwIBAgICAj0wDQYJKoZIhvcNAQEFBQAwZjEaMBgGA1UEAwwRY2Eu
        ...
        -----END CERTIFICATE-----
    </epcert>
    ...

    <!-- List of CA certificates in PEM format -->
    <cacert>
        -----BEGIN CERTIFICATE-----
        MIICzDCCAbSgAwIBAgICA+gwDQYJKoZIhvcNAQELBQAwKTENMAsGA1UECgwEbm9u
        ...
        -----END CERTIFICATE-----
    </cacert>
    ...

    <!-- List of UTM certificates in PEM format and UTM IDs.
         Can have different certificates with the same UTM ID -->
    <utmcert utmid="234566675675ABCF67">
        -----BEGIN CERTIFICATE-----
        MIIC8TCCAdmgAwIBAgICA+gwDQYJKoZIhvcNAQELBQAwKTENMAsGA1UECgwEbm9u
        ...
        -----END CERTIFICATE-----
    </utmcert>

  <!-- List of Exclusions - Entries can be path, process or IP-->
  <exclusions>
    <pathSet>
      <path>c:\test</path>
      <path>notepad.exe</path>
      <path>12.123.123.12</path>
    </pathSet>
    <!-- Boolean if DNS is enabled (true by default) -->
    <dnsEnabled>true</dnsEnabled>
  </exclusions>
</policy>

"""
    def __init__(self):
        self.m_policies = {}

    def getPolicy(self, app, policyID):
        policy = self.m_policies.get(policyID)
        if policy is not None:
            try:
                logger.debug("Sending policy")
                logger.debug(policy)
            except Exception as ude:
                logger.warning(ude.message)
            return policy

        logger.error("No policy found for %s / %s", app, policyID)
        return None

    def addPolicy(self, policyID, policy):
        assert policy is not None
        assert policy != ""
        self.m_policies[policyID] = policy

GL_POLICIES = Policies()

CA_CERT_FILENAME = "root-ca.crt.pem"
UTM_CERT_FILENAME = "utm.cert.pem"

POLICY_BASE = r"""<policy RevID="">
    <destination address="127.0.0.1" port="4444"/>
    <enabled></enabled>
    <renewalparams triggerDaysBefore="90" switchDaysAfter="4" />
    <exclusions>
        <dnsEnabled>true</dnsEnabled>
    </exclusions>
</policy>
"""

HEARTBEAT_ENABLED = True
ERROR_NEXT = False
SERVER_401 = False
NULL_NEXT = False
SERVER_500 = False


def readCert(basename):
    basedir = getCloudAutomationDirectory()
    path = os.path.join(basedir, basename)
    if os.path.isfile(path):
        return open(path).read()

    subprocess.call(["make", basename], cwd=basedir)
    assert os.path.isfile(path)
    return open(path).read()


def generatePolicy(revID, endpointCertMap, caCertList, utmCertMap):
    policy = xml.dom.minidom.parseString(POLICY_BASE)
    policyNode = policy.getElementsByTagName("policy")[0]
    policyNode.setAttribute("RevID", revID)

    exclusionsNode = policyNode.getElementsByTagName("exclusions")[0]

    node = policy.getElementsByTagName("enabled")[0]
    if HEARTBEAT_ENABLED:
        textNode = policy.createTextNode("true")
    else:
        textNode = policy.createTextNode("false")
    node.appendChild(textNode)

    for (fingerprint, cert) in endpointCertMap.items():
        node = policy.createElement("epcert")
        node.setAttribute("fingerprint", fingerprint)
        textNode = policy.createTextNode(cert)
        node.appendChild(textNode)
        policyNode.insertBefore(node, exclusionsNode)

    for cert in caCertList:
        node = policy.createElement("cacert")
        textNode = policy.createTextNode(cert)
        node.appendChild(textNode)
        policyNode.insertBefore(node, exclusionsNode)

    for utmid, cert in utmCertMap.items():
        node = policy.createElement("utmcert")
        node.setAttribute("utmid", utmid)
        textNode = policy.createTextNode(cert)
        node.appendChild(textNode)
        policyNode.insertBefore(node, exclusionsNode)

    text = policy.toxml()
    policy.unlink()
    return text

# HB Policy
class HeartbeatEndpointManager(object):
    def __init__(self):
        self.__m_policyID = "INITIAL_HBT_POLICY_ID"
        self.__m_ca_cert = readCert(CA_CERT_FILENAME)
        self.__m_utm_cert = readCert(UTM_CERT_FILENAME)
        self.__m_signedClientCerts = {}
        self.__m_signedCertCounter = 0
        self.__m_client_pems = []
        GL_POLICIES.addPolicy(
            self.__m_policyID,
            generatePolicy(
                self.__m_policyID,
                {},
                [self.__m_ca_cert],
                {"1": self.__m_utm_cert})
            )

    def policyPending(self):
        return self.__m_policyID is not None

    def policyID(self):
        return self.__m_policyID

    def commandDeleted(self):
        self.__m_policyID = None

    def updatePolicy(self):
        self.__m_policyID = "CLIENT_CERT_HBT_POLICY_ID" + str(random.randrange(0, 101, 2))
        GL_POLICIES.addPolicy(
            self.__m_policyID,
            generatePolicy(
                self.__m_policyID,
                self.__m_signedClientCerts,
                [self.__m_ca_cert],
                {"1":self.__m_utm_cert})
            )

    def updateStatus(self, status, configuration):
        """
        Look at the status and see if we are ready for the next step
        """

        basedir = getCloudAutomationDirectory()
        logger.info("HBT update status=%s, configuration=%s", status, configuration)
        status_xml = xml.dom.minidom.parseString(status)
        addcertNodes = status_xml.getElementsByTagName("addcert")
        if len(addcertNodes) != 1:
            if HEARTBEAT_ENABLED:
                logger.error("Status doesn't contain 1 addcert node")
            return None
        addcertNode = addcertNodes[0]
        for pemNode in addcertNode.getElementsByTagName("pem"):
            pem = getText(pemNode)
            if pem in self.__m_client_pems:
                logger.error("Server received a cert from the endpoint that the server has already seen and signed.")
                raise Exception("Already signed the cert.")
            self.__m_client_pems.append(pem)
            logger.info("Client CSR to sign: %s", pem)
            csrPath = os.path.join(TESTTMP, "csr.pem")
            open(csrPath, "w").write(pem)
            destPath = os.path.join(TESTTMP, "clientCert.pem")
            subprocess.check_call(
                [
                    openssl(), "x509", "-sha256",
                    "-req", "-in", csrPath,
                    "-CA", "root-ca.crt.pem",
                    "-CAkey", "root-ca.key.pem",
                    "-CAcreateserial",
                    "-out", destPath,
                    "-days", "365"],
                cwd=basedir)
            self.__m_signedClientCerts["FP%d"%(self.__m_signedCertCounter)] = open(destPath).read()
            self.__m_signedCertCounter += 1


        if len(self.__m_signedClientCerts) == 0:
            logger.debug("No client csr to sign")
            return

        action_log.debug("SIGNED CLIENT CSR")
        self.__m_policyID = "CLIENT_CERT_HBT_POLICY_ID"
        GL_POLICIES.addPolicy(
            self.__m_policyID,
            generatePolicy(
                self.__m_policyID,
                self.__m_signedClientCerts,
                [self.__m_ca_cert],
                {"1":self.__m_utm_cert})
            )
        return None

# AV Policy
class SAVEndpointManager(object):
    def __init__(self):
        self.__m_policyID = "INITIAL_SAV_POLICY_ID"
        self.__m_policy = INITIAL_SAV_POLICY
        GL_POLICIES.addPolicy(self.__m_policyID, self.__m_policy)
        self.__m_scanNow = None
        self.__m_clearAction = None

    def policyPending(self):
        return self.__m_policyID is not None
        
    def scanNowPending(self):
        return self.__m_scanNow is not None

    def clearActionPending(self):
        ret =  self.__m_clearAction is not None
        if ret:
            self.__m_clearAction = None
        return ret

    def policyID(self):
        return self.__m_policyID

    def commandDeleted(self):
        self.__m_policyID = None
        self.__m_scanNow = None

    def setOnAccess(self, enable):
        ## get current value
        dom = xml.dom.minidom.parseString(self.__m_policy)
        onAccessScanNode = dom.getElementsByTagName("onAccessScan")[0]
        enabledNode = onAccessScanNode.getElementsByTagName("enabled")[0]
        enabledNode.normalize()
        enabledString = getText(enabledNode)
        current = enabledString.lower() == "true"

        ## if same as new value return
        if current == enable:
            dom.unlink()
            return

        ## edit XML
        newValue = "true" if enable else "false"
        enabledNode.firstChild.data = newValue

        policyXml = dom.toxml("UTF-8")
        dom.unlink()
        ## updatePolicy
        self.updatePolicy(policyXml)

    def updatePolicy(self, policy):
        self.__m_policy = policy
        self.__m_policyID = "SAV%f"%(time.time())
        logger.info("Updating SAV policy: %s",self.__m_policyID)
        GL_POLICIES.addPolicy(self.__m_policyID, self.__m_policy)
    
    def scanNow(self):
        logger.info("Triggering an on demand scan action")
        self.__m_scanNow = True
    def clearNonAsciiAction(self):
        logger.info("Triggering an on demand clear action")
        self.__m_clearAction = True


class EDREndpointManager(object):
    def __init__(self):
        self.__livequery = ""
        self.__id = ""

    def liveQueryPending(self):
        return self.__livequery != ""

    def setQuery(self, query, command_id):
        self.__livequery = query
        self.__id = command_id

    def liveQuery(self):
        if self.__livequery == "":
            raise AssertionError("No LiveQuery available")
        return self.__livequery, self.__id

    def clearLiveQuery(self):
        self.__livequery = ""
        self.__id = ""


class LiveTerminalEndpointManager(object):
    def __init__(self):
        self.__liveTerminalInit = ""
        self.__id = ""

    def LiveTerminalPending(self):
        return self.__liveTerminalInit != ""

    def initiateLiveTerminal(self, cmd_body, command_id):
        self.__id = command_id
        if not cmd_body:
            self.__liveTerminalInit = \
                """<action type="sophos.mgt.action.InitiateLiveTerminal"><url>"wss://url"</url><thumbprint>thumbprint</thumbprint></action>"""
        else:
            self.__liveTerminalInit = cmd_body.decode("utf-8")

    def liveTerminal(self):
        if self.__liveTerminalInit == "":
            raise AssertionError("No LiveTerminal init command available")
        return self.__liveTerminalInit, self.__id

    def clearLiveTerminal(self):
        self.__liveTerminalInit = ""
        self.__id = ""

# MCS Policy
class MCSEndpointManager(object):
    def __init__(self):
        self.__m_policyID = "INITIAL_MCS_POLICY_ID"
        self.__m_policy = INITIAL_MCS_POLICY
        GL_POLICIES.addPolicy(self.__m_policyID, self.__m_policy)

    def policyPending(self):
        return self.__m_policyID is not None

    def policyID(self):
        return self.__m_policyID

    def commandDeleted(self):
        self.__m_policyID = None

    def updatePolicy(self, body):
        self.__m_policyID = "MCS%f"%(time.time())
        self.__m_policy = body
        logger.info("Updating MCS policy: %s",self.__m_policyID)
        GL_POLICIES.addPolicy(self.__m_policyID, self.__m_policy)

    def getPolicy(self):
        return self.__m_policy

# ALC Policy
class ALCEndpointManager(object):
    def __init__(self):
        self._counter = 0
        self.__m_policyID = ["INITIAL_ALC_POLICY_ID"]
        GL_POLICIES.addPolicy(self.__m_policyID[0], INITIAL_ALC_POLICY)
        self.__m_updateNow = None
        self._extraPolicies = []

    def policyPending(self):
        return len(self.__m_policyID) != 0

    def policiesID(self):
        return self.__m_policyID

    def commandDeleted(self):
        self.__m_policyID = []
        self.__m_updateNow = None
    
    def updateNowPending(self):
        return self.__m_updateNow is not None
    
    def updateNow(self):
        logger.info("Triggering an update now action")
        self.__m_updateNow = True

    def updatePolicy(self, body):
        self._counter += 1
        newId = "ALC{}{}".format(time.time(),self._counter)
        self.__m_policyID.append(newId)
        logger.info("Updating ALC policy: %s",newId)
        GL_POLICIES.addPolicy(newId, body)


# MDR Policy
class MDREndpointManager(object):
    def __init__(self):
        self.__m_policyID = "INITIAL_MDR_POLICY_ID"
        self.__m_policy = INITIAL_MDR_POLICY
        GL_POLICIES.addPolicy(self.__m_policyID, self.__m_policy)

    def policyPending(self):
        return self.__m_policyID is not None

    def policyID(self):
        return self.__m_policyID

    def commandDeleted(self):
        self.__m_policyID = None

    def updatePolicy(self, body):
        self.__m_policyID = "MCS%f"%(time.time())
        self.__m_policy = body
        logger.info("Updating MCS policy: %s",self.__m_policyID)
        GL_POLICIES.addPolicy(self.__m_policyID, self.__m_policy)

    def getPolicy(self):
        return self.__m_policy

class Endpoint(object):
    def __init__(self, status):
        global ID
        self.__m_id = "ThisIsAnMCSID+%d"%ID
        ID += 1
        self.__hb = HeartbeatEndpointManager()
        self.__sav = SAVEndpointManager()
        self.__edr = EDREndpointManager()
        self.__liveTerminal = LiveTerminalEndpointManager()
        self.__mcs = MCSEndpointManager()
        self.__alc = ALCEndpointManager()
        self.__mdr = MDREndpointManager()
        self.__m_doc = None
        self.__m_health = 0
        self.__m_updatesource = None
        self.updateStatus("AGENT", status)

        # Useful for specific test scenarios
        self.queued_actions = []

    def updateStatus(self, app, status):
        #~ logger.debug("%s status is %s", app, status)
        if app == "AGENT":
            if self.__m_doc is not None:
                self.__m_doc.unlink()
                self.__m_doc = None
            self.__m_doc = xml.dom.minidom.parseString(status)
            nameNodes = self.__m_doc.getElementsByTagName("computerName")
            if len(nameNodes) == 0:
                logger.error("Can't understand status")
                return
            self.__m_name = getText(nameNodes[0])
        elif app == "HBT":
            logger.info("Heartbeat status = %s", status)
            doc = xml.dom.minidom.parseString(status.strip())
            statusNode = doc.getElementsByTagName("status")[0]
            status = getText(statusNode).strip()
            #~ status = xml.sax.saxutils.unescape(status)
            configurationNode = doc.getElementsByTagName("configuration")[0]
            configuration = getText(configurationNode).strip()
            #~ configuration = xml.sax.saxutils.unescape(configuration)
            self.__hb.updateStatus(status, configuration)
        elif app == "SHS":
            logger.info("Health status = %s", status)
            # Example XML:
            #   <?xml version="1.0" encoding="utf-8"?>
            #   <health activeHeartbeat="true" activeHeartbeatUtmId="000001" version="3.0.0">
            #       <item name="health" value="1"/>
            #       <item name="service" value="1"/>
            #       <item name="threat" value="1"/>
            #   </health>

            doc = xml.dom.minidom.parseString(status.strip())
            statusNode = doc.getElementsByTagName("status")[0]
            statusNodeText = getText(statusNode).strip()
            innerStatusNodeXml = xml.dom.minidom.parseString(statusNodeText)
            healths = innerStatusNodeXml.getElementsByTagName("health")

            if healths.length != 1:
                logger.error("SHS Status doesn't contain 1 health node.")
                return None

            health = healths[0]
            items = health.getElementsByTagName("item")
            for item in items:
                if item.attributes["name"].value == "health":
                    self.__m_health = item.attributes["value"].value
                    logger.info("Endpoint health status set to %s", self.__m_health)
        elif app in ["ALC", "SAV", "NTP", "APPSPROXY", "MCS", "MDR"]:
            logger.info("{} status = {}".format(app, status))
        else:
            logger.error("Attempting to update status for unknown app: %s", app)

    def handleEvent(self, app, event):
        logger.info("{} event = {}".format(app, event))
        if app == "ALC":
            logger.info("ALC event = %s", event)

            """
            <?xml version="1.0"?>
            <event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
              <timestamp>{{timestamp}}</timestamp>
              <appInfo>
                <number>{{message number}}</number>
                <message>
                  <message_inserts>
                    {{inserts}}
                  </message_inserts>
                </message>
              <updateSource>{{source}}</updateSource>
              </appInfo>
              <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
            </event>
            """

            doc = xml.dom.minidom.parseString(event.strip())
            eventNode = doc.getElementsByTagName("event")[0]
            updateSourceNodes = eventNode.getElementsByTagName("updateSource")
            if updateSourceNodes.length != 1:
                logger.error("ALC event doesn't contain 1 updateSource node.")
                self.__m_updatesource = None
                return
            updateSourceNode = updateSourceNodes[0]
            updateSource = getText(updateSourceNode).strip()
            self.__m_updatesource = updateSource
            logger.info("Endpoint updateSource is reported as: %s", updateSource)

            return
        else:
            logger.error("Attempting to handle event for unknown app: %s", app)

    def handle_response(self, app_id, correlation_id, response_body):
        # TODO - LINUXDAR-922 revert this when central supports compression
        decompressed_body = response_body
        # try:
        #     fake_file = StringIO.StringIO(response_body)
        #     decompressed_fake_file = gzip.GzipFile(fileobj=fake_file, mode='rb')
        #     decompressed_body = decompressed_fake_file.read()
        #
        # except Exception as e:
        #     logger.error("Failed to decompress response body content: {}".format(e))
        #     return
        logger.info("{} response ({}) = {}".format(app_id, correlation_id, decompressed_body.decode()))
        with open(os.path.join(_get_log_dir(), "last_query_response.json"), 'w') as live_query_response_file:
            live_query_response_file.write(decompressed_body.decode())


    def name(self):
        return self.__m_name

    def getHealth(self):
        return self.__m_health

    def getUpdateSource(self):
        return self.__m_updatesource

    def id(self):
        return self.__m_id

    def getElementText(self, nodeName):
        element = self.__m_doc.getElementsByTagName(nodeName)[0]
        return getText(element)

    def commands(self):
        return []

    def emptyCommand(self):
        return r"""<?xml version="1.0"?>
<ns:commands xmlns:ns="http://www.sophos.com/xml/mcs/commands" schemaVersion="1.0">
</ns:commands>"""

    def policyAssignment(self, app, policyID):
        return r"""<?xml version="1.0"?>
<ns:policyAssignments xmlns:ns="http://www.sophos.com/xml/mcs/policyAssignments">
  <meta protocolVersion="1.0"/>
  <policyAssignment>
    <appId>%s</appId>
    <policyId>%s</policyId>
  </policyAssignment>
</ns:policyAssignments>
"""%(app, policyID)

    def policyCommand(self, app, policyID):
        policyAssignment = self.policyAssignment(app, policyID)
        return r"""<command>
    <id>%s</id>
    <seq>1</seq>
    <appId>APPSPROXY</appId>
    <creationTime>2013-05-02T09:50:08Z</creationTime>
    <ttl>PT10000S</ttl>
    <body>%s</body>
  </command>"""%(app, xml.sax.saxutils.escape(policyAssignment))
  
    def updateNowCommand(self, creation_time="FakeTime"):
        body = r"""<?xml version='1.0'?><action type="sophos.mgt.action.ALCForceUpdate"/>"""
        
        return r"""<command>
        <id>ALC</id>
        <seq>1</seq>
        <appId>ALC</appId>
        <creationTime>{}</creationTime>
        <ttl>PT10000S</ttl>
        <body>{}</body>
      </command>""".format(creation_time, xml.sax.saxutils.escape(body))
      
    def scanNowCommand(self):
        body = r"""<?xml version="1.0"?><a:action xmlns:a="com.sophos/msys/action" type="ScanNow" id="" subtype="ScanMyComputer" replyRequired="1"/>"""
        
        return r"""<command>
        <id>SAV</id>
        <seq>1</seq>
        <appId>SAV</appId>
        <creationTime>FakeTime</creationTime>
        <ttl>PT10000S</ttl>
        <body>%s</body>
      </command>"""%(xml.sax.saxutils.escape(body))

    def clearNonAsciiCommand(self):
        try:
            nonascii = chr(169) + chr(40960) + chr(1972) + chr(20013) + chr(70000)
            templatebody = """&lt;?xml version=&quot;1.0&quot;?&gt;&lt;a:action xmlns:a=&quot;com.sophos/msys/action&quot; type=&quot;sophos.mgt.action.SAVClearFromList&quot;&gt;&lt;threat-set&gt;&lt;threat id=&quot;T2398541734956424221&quot; idSource=&quot;Tmd5(dis,vName,path)&quot; item=&quot;/tmp/%s&quot; name=&quot;VirusFile&quot; type=&quot;virus&quot; typeId=&quot;1&quot;/&gt;&lt;/threat-set&gt;&lt;/a:action&gt;"""
            body = templatebody%(nonascii)
            body = body.encode('utf-8', 'replace')
            content = """<command>
            <id>56</id>
            <seq>1</seq>
            <appId>SAV</appId>
            <creationTime>FakeTime</creationTime>
            <ttl>PT10000S</ttl>
            <body>%s</body>
          </command>"""%(body.decode('utf-8'))
            return content
        except:
            import traceback
            logger.error( traceback.format_exc())

    def liveQueryCommand(self):
        body, id = self.__edr.liveQuery()
        self.__edr.clearLiveQuery()
        now = datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.%fZ")
        return r"""<command>
        <id>{}</id>
        <appId>LiveQuery</appId>
        <creationTime>{}</creationTime>
        <ttl>PT10000S</ttl>
        <body>{}</body>
      </command>""".format(id, now, xml.sax.saxutils.escape(body.decode("utf-8")))

    def liveTerminalCommand(self):
        body, id = self.__liveTerminal.liveTerminal()
        self.__liveTerminal.clearLiveTerminal()
        now = datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.%fZ")
        return r"""<command>
        <id>{}</id>
        <appId>LiveTerminal</appId>
        <creationTime>{}}</creationTime>
        <ttl>PT10000S</ttl>
        <body>{}</body>
      </command>""".format(id, now, xml.sax.saxutils.escape(body))

    def commandXml(self, apps):
        logger.debug("commandXML - %s", apps)
        commands = []
        if "HBT" in apps and self.__hb.policyPending():
            commands.append(self.policyCommand("HBT", self.__hb.policyID()))
        if "SAV" in apps and self.__sav.policyPending():
            commands.append(self.policyCommand("SAV", self.__sav.policyID()))
        if "SAV" in apps and self.__sav.scanNowPending():
            commands.append(self.scanNowCommand())
        if "SAV" in apps and self.__sav.clearActionPending():
            commands.append(self.clearNonAsciiCommand())
        if 'LiveQuery' in apps and self.__edr.liveQueryPending():
            commands.append(self.liveQueryCommand())
        if "MCS" in apps and self.__mcs.policyPending():
            commands.append(self.policyCommand("MCS", self.__mcs.policyID()))
        if "ALC" in apps and self.__alc.policyPending():
            for policy_id in self.__alc.policiesID():
                commands.append(self.policyCommand("ALC", policy_id))
        if "ALC" in apps and self.__alc.updateNowPending():
            commands.append(self.updateNowCommand())
        if "MDR" in apps and self.__mdr.policyPending():
            commands.append(self.policyCommand("MDR", self.__mdr.policyID()))
        if 'LiveTerminal' in apps and self.__liveTerminal.LiveTerminalPending():
            commands.append(self.liveTerminalCommand())

        # Useful for specific test scenarios
        commands.extend(self.queued_actions)
        self.queued_actions = []

        output = r"""<?xml version="1.0"?>
<ns:commands xmlns:ns="http://www.sophos.com/xml/mcs/commands" schemaVersion="1.0">"""
        for command in commands:
            output += command
        output += r"""</ns:commands>"""
        return output

    def deleteCommands(self, commandIDs):
        for c in commandIDs:
            if c == "HBT":
                self.__hb.commandDeleted()
            elif c == "SAV":
                self.__sav.commandDeleted()
            elif c == "MCS":
                self.__mcs.commandDeleted()
            elif c == "ALC":
                self.__alc.commandDeleted()
            elif c == "MDR":
                self.__mdr.commandDeleted()
            else:
                logger.error("Attempting to delete unknown command: %s", c)

    def updateHbPolicy(self):
        self.__hb.updatePolicy()

    def updateMcsPolicy(self, body):
        self.__mcs.updatePolicy(body)

    def updateMdrPolicy(self, body):
        self.__mdr.updatePolicy(body)

    def updateSavPolicy(self, body):
        self.__sav.updatePolicy(body)

    def updatePolicy(self, adapter, body):
        if adapter == "SAV":
            self.__sav.updatePolicy(body)
        elif adapter == "MCS":
            self.__mcs.updatePolicy(body)
        elif adapter == "ALC":
            self.__alc.updatePolicy(body)
        elif adapter == "MDR":
            self.__mdr.updatePolicy(body)
            
    def updateNow(self):
        self.__alc.updateNow()

    def setQuery(self, query, command_id):
        self.__edr.setQuery(query, command_id)

    def initiateLiveTerminal(self, body, command_id):
        self.__liveTerminal.initiateLiveTerminal(body, command_id)

    def queueUpdateNow(self, creation_time):
        self.queued_actions.append(self.updateNowCommand(creation_time))

    def scanNow(self):
        self.__sav.scanNow()

    def clearNonAsciiAction(self):
        self.__sav.clearNonAsciiAction()

    def getPolicy(self, adapter):
        if adapter == "MCS":
            return self.__mcs.getPolicy()
        return None

    def setOnAccess(self, enable):
        return self.__sav.setOnAccess(enable)

    def report(self):
        return {
                'name':self.name(),
                'id':self.id(),
                'on_access':False,
                'status':{},
                }

    def server(self):
        return {
                'name':self.name(),
                'id':self.id(),
                'status':{},
                }


class Endpoints(object):
    def __init__(self):
        self.__m_endpoints = {}

    def register(self, status):
        logger.debug("REGISTER BODY:%s", status)
        action_log.debug("REGISTER")
        e = Endpoint(status)
        self.__m_endpoints[e.name()] = e
        return e.id()

    def computers(self):
        results = []
        for e in self.__m_endpoints.values():
            results.append(e.server())
        return results

    def computer(self, eid):
        e = self.getEndpointByID(eid)
        if e is not None:
            return e.server()
        return {}

    def reports(self):
        results = []
        for e in self.__m_endpoints.values():
            results.append(e.report())
        return results

    def getEndpointByID(self, eid):
        for e in self.__m_endpoints.values():
            if e.id() == eid:
                return e
        return None

    def getEndpointByHostname(self, hostname):
        for e in self.__m_endpoints.values():
            if e.name() == hostname:
                return e
        return None

    def commands(self, eid):
        e = self.getEndpointByID(eid)
        if e is None:
            return []
        else:
            return e.commands()

    def commandXml(self, eid, apps):
        action_log.debug("COMMANDS")
        endpoint = self.getEndpointByID(eid)
        if endpoint is None:
            return None
        return endpoint.commandXml(apps)

    def deleteCommands(self, eid, commandIDs):
        action_log.debug("DELETE COMMANDS")
        endpoint = self.getEndpointByID(eid)
        if endpoint is None:
            logger.error("Get deleteCommands for unknown endpoint: %s", eid)
            return None
        return endpoint.deleteCommands(commandIDs)

    def updateStatus(self, eid, status):
        action_log.debug("STATUS")
        endpoint = self.getEndpointByID(eid)
        if endpoint is None:
            return None
        return endpoint.updateStatus(status)

    def updateHb(self):
        for e in self.__m_endpoints.values():
            e.updateHbPolicy()

    def updateMcsPolicy(self, policy):
        return self.updatePolicy("MCS",policy)

    def getPolicy(self, adapter):
        for e in self.__m_endpoints.values():
            return e.getPolicy(adapter)

    def updatePolicy(self, adapter, policy):
        assert policy is not None
        assert policy != ""
        for e in self.__m_endpoints.values():
            e.updatePolicy(adapter, policy)

    def setQuery(self, adapter, query, command_id):
        logger.info("LiveQuery command {}".format(query))
        assert query != ""
        assert adapter == "LiveQuery"
        for e in self.__m_endpoints.values():
            e.setQuery(query, command_id)

    def setOnAccess(self, enable):
        for e in self.__m_endpoints.values():
            e.setOnAccess(enable)

    def queueUpdateNow(self, creation_time):
        for e in self.__m_endpoints.values():
            e.queueUpdateNow(creation_time)

    def updateNow(self):
        for e in self.__m_endpoints.values():
            e.updateNow()
    
    def scanNow(self):
        for e in self.__m_endpoints.values():
            e.scanNow()
    def clearNonAsciiAction(self):
        for e in self.__m_endpoints.values():
            e.clearNonAsciiAction()

    def initiateLiveTerminal(self, adapter="LiveTerminal", body=None, command_id="LiveTerminal"):
        logger.info("LiveTerminal command {}".format(body))
        assert body != ""
        assert adapter == "LiveTerminal"
        for e in self.__m_endpoints.values():
            e.initiateLiveTerminal(body, command_id)


GL_ENDPOINTS = Endpoints()

class MCSHandler(object):
    pass

COOKIE = None
AWSCOOKIE = None

def reset_cookies():
    global COOKIE
    global AWSCOOKIE
    ran = random.randrange(2**128)
    ran1 = random.randrange(2**512)

    COOKIE = "SID=%032x"%ran
    logger.debug("NEW Cookie: %s", COOKIE)
    AWSCOOKIE = "AWSALB=%124x"%ran1
    logger.debug("NEW Cookie: %s", AWSCOOKIE)

REREGISTER_NEXT = False
REGISTER_401 = False

class MCSRequestHandler(http.server.BaseHTTPRequestHandler, object):
    """
    HTTPS server
        - localhost
        - how to verify?
            - replace CA in install?
    registration
        - fixed client ID?
    check status
        - use for testing status messages?
    send Heartbeat policy with fake UTM info
        - need certificates for UTM and CA
    Response to certificate request
        - Need openssl functionality/calls
    """

    def __init__(self, request, client_address, server):
        http.server.BaseHTTPRequestHandler.__init__(self, request, client_address, server)

    def send_cookie(self):
        ## only send cookie to the MCS client
        if self.path.startswith("/mcs") and COOKIE is not None:
            logger.debug("SEND Cookie: %s", COOKIE + "; Path=/; Secure; HttpOnly")
            self.send_header("Set-Cookie", COOKIE + "; Path=/; Secure; HttpOnly")

    def sendAWSCookie(self):
        ## only send cookie to the MCS client
        if self.path.startswith("/mcs") and AWSCOOKIE is not None:
            logger.debug("SEND Cookie: %s", AWSCOOKIE + "; Expires=Fri, 18 May 2020 10:25:17 GMT; Path=/")
            self.send_header("Set-Cookie", AWSCOOKIE + "; Expires=Fri, 18 May 2020 10:25:17 GMT; Path=/")

    def ret(self, message=None, code=200, extra_header={}):
        if message is None:
            message = ""

        self.send_response(code, "OK")
        try:
            logger.debug(message)
        except Exception as e:
            logger.warning(e.message)
        try:
            if isinstance(message, str):
                message = message.encode('utf-8', 'replace')
            else:
                message = message.decode('utf-8', 'replace').encode('utf-8')
        except Exception as ex:
            logger.warning(ex.message)
        self.send_header("Content-Length", str(len(message)))
        self.sendAWSCookie()
        self.send_cookie()
        for k, v in extra_header.items():
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(message)


    def retJson(self, struct, code=200):
        return self.ret(json.dumps(struct),code=code)

    def deploymentLocations(self):
        message = [
            {
                'platform':"Linux",
                'command':"/opt/sophos-av/engine/register_central.py ThisIsARegToken https://localhost:4443/mcs",
                'url':'http://localhost:4443/thinInstaller',
            },
            ]
        return self.retJson(message)

    def do_GET_frontend(self):
        if self.path == "/frontend/api/deployment/agent/locations?transports=endpoint_mcs":
            return self.deploymentLocations()

        elif self.path.startswith("/frontend/api/servers?"):
            return self.retJson(
                {
                    "items":GL_ENDPOINTS.computers()
                })
        elif self.path.startswith("/frontend/api/servers/"):
            eid = self.path[len("/frontend/api/servers/"):]
            logger.info("Info for %s",eid)
            return self.retJson(GL_ENDPOINTS.computer(eid))
        # Handle any fake/custom queries that are not actually available on the real cloud api
        elif self.path.startswith("/frontend/api/fake/health"):
            parsed = urllib.parse.urlparse(self.path)
            args = urllib.parse.parse_qs(parsed.query)['hostname']
            if len(args) == 1:
                hostname = args[0]
                health = GL_ENDPOINTS.getEndpointByHostname(hostname).getHealth()
                return self.retJson(
                    {
                        "health":health,
                    })
        elif self.path.startswith("/frontend/api/fake/updatesource"):
            parsed = urllib.parse.urlparse(self.path)
            args = urllib.parse.parse_qs(parsed.query)['hostname']
            if len(args) == 1:
                hostname = args[0]
                updatesource = GL_ENDPOINTS.getEndpointByHostname(hostname).getUpdateSource()
                return self.retJson(
                    {
                        "updatesource":updatesource,
                    })

        elif self.path.startswith("/frontend/api/policies/servers"):
            return self.retJson([])

        elif self.path.startswith("/frontend/api/reports/servers"):
            return self.retJson(
                {
                    "items":GL_ENDPOINTS.reports()
                })

        logger.warn("unknown do_GET_frontend %s", self.path)
        return self.retJson("frontend-api-not-understood %s"%self.path)

    def mcs_commands(self):
        #~ logger.info("mcs_commands: %s", self.path)
        mo = re.match(r"/mcs/commands/applications/([^/]+)/endpoint/(.*)$", self.path)
        if not mo:
            logger.error("Failed to parse: %s", self.path)
            return self.ret("Unknown mcs_commands", code=500)
        apps = mo.group(1).split(";")
        eid = mo.group(2)
        logger.info("mcs_commands: eid=%s apps=%s", eid, str(apps))
        commands = GL_ENDPOINTS.commandXml(eid, apps)
        if commands is None:
            logger.error("Unknown eid %s", eid)
            return self.ret("Unknown mcs_commands", code=500)

        return self.ret(commands)

    def mcs_policy(self):
        mo = re.match(r"/mcs/policy/application/([^/]+)/([^/]+)", self.path)
        logger.info("Requesting policy - %s   %s", mo.group(1), mo.group(2))
        policy = GL_POLICIES.getPolicy(mo.group(1), mo.group(2))
        if policy is not None:
            return self.ret(policy)
        else:
            logger.error("Requested non-existent policy")
            return self.ret("Unknown policy", code=500)

    def push_redirect(self):
        logger.info("Push redirect requested. headers received: {}".format(dict(self.headers)))
        auth = self.headers['Authorization']
        logger.info("Value for auth: {}".format(auth))
        if 'Basic' not in auth:
            logger.info("Refusing to redirect as unauthorized")
            return self.ret("Unauthorized access to push server", code=401)
        return self.ret(code=307, extra_header={'Location': 'https://localhost:8459{}'.format(self.path)})

    def send_401(self):
        action_log.debug("401")
        logger.debug("Sending 401 for %s", self.path)
        self.send_response(401, "UNAUTHORIZED")
        self.send_header("www-authenticate", 'Basic realm="register"')
        self.send_header("Content-Length", "0")
        self.sendAWSCookie()
        self.send_cookie()
        self.end_headers()
        return True

    def do_GET_hb(self):
        global HEARTBEAT_ENABLED

        if self.path == "/hbdisable":
            HEARTBEAT_ENABLED = False
            GL_ENDPOINTS.updateHb()
            return self.ret("")
        elif self.path == "/hbenable":
            HEARTBEAT_ENABLED = True
            GL_ENDPOINTS.updateHb()
            return self.ret("")
        else:
            return self.ret("Unknown Heartbeat path, should be hbdisable or hbdisable.", code=500)

    def do_GET_error(self):
        global ERROR_NEXT
        global SERVER_401
        global SERVER_500
        global NULL_NEXT

        if self.path == "/error":
            ERROR_NEXT = True
            return self.ret("")
        elif self.path == "/error/server401":
            logger.info("SENDING 401s")
            SERVER_401 = True
            return self.ret("")
        elif self.path == "/error/server500":
            logger.info("SENDING 500")
            SERVER_500 = True
            return self.ret("")
        elif self.path == "/error/null":
            logger.info("SENDING empty command response")
            NULL_NEXT = True
            return self.ret("")
        else:
            return self.ret("Unknown Error command path, should be /error", code=500)
            
    def do_GET_action(self):
        if self.path == "/action/updatenow":
            logger.info("Received update now trigger")
            GL_ENDPOINTS.updateNow()
            return self.ret("")
        if self.path.startswith("/action/queueupdatenow"):
            logger.info("Received queue update now trigger")
            creation_time = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query).get('creationtime', ["FakeTime"])[0]
            GL_ENDPOINTS.queueUpdateNow(creation_time)
            return self.ret("")
        elif self.path == "/action/scannow" :
            logger.info("Received on demand scan trigger")
            GL_ENDPOINTS.scanNow()
            return self.ret("")
        elif self.path == "/action/clearNonAsciiAction":
            logger.info("Received on demand clear now non ascii action ")
            GL_ENDPOINTS.clearNonAsciiAction()
            return self.ret("")
        elif self.path == "/action/initiateLiveTerminal":
            logger.info("Received an initiate live-terminal action")
            GL_ENDPOINTS.initiateLiveTerminal()
            return self.ret("")
        else:
            return self.ret("Unknown Action command path, should be /action/*", code=500)

    def send_error_503(self):
        action_log.debug("503")
        logger.debug("Sending 503 for %s", self.path)
        self.send_response(503, "HTTP Service Unavailable")
        self.send_header("Content-Length", "0")
        ## reset the cookie (this will be ignored by the client in this response)
        reset_cookies()
        self.sendAWSCookie()
        self.send_cookie()
        self.end_headers()
        return True

    def do_GET_mcs(self):
        global ERROR_NEXT
        global REREGISTER_NEXT
        global NULL_NEXT

        ua = self.headers.get("User-Agent", "<unknown>").split('/', 1)[0]
        logger.debug("GET - %s (%s); Cookie=", self.path, ua)
        logger.debug("RECV Cookie: %s", self.headers.get("Cookie", "<none>"))
        self.verify_cookies()

        MCSRequestHandler.m_userAgent = self.headers.get("User-Agent", "<unknown>")

        if self.path == "/mcs":
            return self.ret("")
        elif MCSRequestHandler.options.reregister:
            return self.send_401()
        elif REREGISTER_NEXT and not self.path.startswith('/mcs/push/endpoint/'):
            REREGISTER_NEXT = False
            return self.send_401()
        elif ERROR_NEXT and not self.path.startswith('/mcs/push/endpoint/'):
            ERROR_NEXT = False
            return self.send_error_503()
        elif self.path.startswith("/mcs/commands/applications/"):
            if NULL_NEXT:
                NULL_NEXT = False
                return self.ret("")
            return self.mcs_commands()
        elif self.path.startswith("/mcs/policy/application/"):
            return self.mcs_policy()
        elif self.path.startswith('/mcs/push/endpoint/'):
            return self.push_redirect()

        return self.ret("Unknown MCS verb", code=500)

    def verify_cookies(self):
        if not MCSRequestHandler.options.verifycookies:
            return

        # only verify requests from MCS
        ua = self.headers.get("User-Agent", None)
        if ua is None:
            return
        if not ua.startswith("Sophos MCS Client"):
            return

        cookie = self.headers.get("Cookie", None)
        if cookie is None:
            return
        if COOKIE in cookie:
            if AWSCOOKIE in cookie:
                logger.debug("Cookies are good!")
                return

        logger.error("Received mismatched cookie from MCS: %s, expected: %s",
                     cookie, COOKIE, AWSCOOKIE)

    def do_GET_controller(self):
        if self.path == "/controller/reregisterNext":
            global REREGISTER_NEXT
            REREGISTER_NEXT = True
            return self.ret("")
        elif self.path == "/controller/onaccess/off":
            GL_ENDPOINTS.setOnAccess(False)
            return self.ret("")
        elif self.path == "/controller/mcs/policy":
            return self.ret(GL_ENDPOINTS.getPolicy("MCS"))
        elif self.path == "/controller/userAgent":
            return self.ret(MCSRequestHandler.m_userAgent)
        elif self.path == "/controller/register401":
            global REGISTER_401
            REGISTER_401 = not REGISTER_401
            return self.ret("")


    def do_GET_dispatcher(self):
        if self.path.startswith("/controller/"):
            return self.do_GET_controller()

        global SERVER_401
        if SERVER_401:
            return self.send_401()
        if self.path.startswith("/frontend/api/"):
            return self.do_GET_frontend()
        elif self.path.startswith("/mcs"):
            return self.do_GET_mcs()

        # Handle any hb custom queries that are not actually available on the real cloud
        elif self.path.startswith("/hb"):
            return self.do_GET_hb()

        elif self.path.startswith("/error"):
            return self.do_GET_error()

        elif self.path == "/":
            return self.ret(json.dumps({}))

        elif self.path.startswith("/action/"):
            return self.do_GET_action()

        logger.warn("unknown do_GET: %s", self.path)
        message = "<html><body>GET REQUEST %s</body></html>"%self.path
        return self.ret(message, code=404)

    def do_GET(self):
        assert os.getppid() == ORIGINAL_PPID
        try:
            return self.do_GET_dispatcher()
        except Exception as e:
            logger.warning("Failed to do do_GET with error: {}".format(e))
            raise

    def zero(self):
        """
        #~ self.upe_api = json_response['apis']['upe']['ng_url']
        #~ self.hammer_token = json_response['token']
        """
        return self.ret(json.dumps(
            {
                "apis":{
                    "upe":{
                        "ng_url":"https://localhost:4443/frontend/api",
                    },
                },
                "token":"FOOBAR",
                "csrf":"01234567901234567890123456789012",
            }))

    def getBody(self):
        size = int(self.headers['Content-Length'])
        body = self.rfile.read(size)
        return body

    def mcs_register(self):
        auth = self.headers['Authorization']
        if auth.startswith("Basic "):
            auth = auth[len("Basic "):]
            auth = base64.b64decode(auth)
            logger.info("Register with %s",auth.decode("latin1"))
        global REGISTER_401
        if REGISTER_401:
            return self.send_401()

        body = self.getBody()
        eid = GL_ENDPOINTS.register(body)

        hash = base64.b64encode("{}:ThisIsThePassword".format(eid).encode('utf-8'))
        return self.ret(hash)

    def mcs_event(self):
        match_object = re.match(r"/mcs/events/endpoint/([^/]+)", self.path)
        if not match_object:
            return self.ret("Bad event path", 400)

        eid = match_object.group(1)
        endpoint = GL_ENDPOINTS.getEndpointByID(eid)
        if endpoint is None:
            ## Create endpoint?
            return self.ret("Event for unknown endpoint", 400)

        body = self.getBody()
        #~ logger.debug("BODY1 %s", body)
        doc = xml.dom.minidom.parseString(body)

        eventsNode = doc.getElementsByTagName("ns:events")[0]
        eventNodes = eventsNode.getElementsByTagName("event")

        for eventNode in eventNodes:
            appNode = eventNode.getElementsByTagName("appId")[0]
            app = getText(appNode)
            bodyNode = eventNode.getElementsByTagName("body")[0]
            body = getText(bodyNode)
            #~ body = xml.sax.saxutils.unescape(body)
            #~ logger.debug("BODY2 %s %s", app, body)
            endpoint.handleEvent(app, body)

        return self.ret("")

    def edr_response(self):
        match_object = re.match(r"^/mcs/responses/endpoint/([^/]*)/app_id/([^/]*)/correlation_id/([^/]*)$", self.path)
        if not match_object:
            return self.ret("Bad response path", 400)

        endpoint_id = match_object.group(1)
        app_id = match_object.group(2)
        correlation_id = match_object.group(3)
        endpoint = GL_ENDPOINTS.getEndpointByID(endpoint_id)
        if endpoint is None:
            ## Create endpoint?
            return self.ret("Response for unknown endpoint", 400)
        if SERVER_500:
            return self.ret("Internal Server Error", 500)

        response_body = self.getBody()
        endpoint.handle_response(app_id, correlation_id, response_body)
        return self.ret("")


    def do_POST_mcs(self):
        if self.path == "/mcs/register":
            return self.mcs_register()
        elif self.path == "/mcs":
            return self.ret("")

        user_agent = self.headers.get("User-Agent", "<unknown>").split('/', 1)[0]
        logger.debug("PUT - %s (%s); Cookie=%s", self.path, user_agent,
            self.headers.get("Cookie", "<none>"))
        self.verify_cookies()

        MCSRequestHandler.m_userAgent = self.headers.get("User-Agent", "<unknown>")
        if self.path.startswith("/mcs/events/endpoint"):
            return self.mcs_event()
        elif self.path.startswith("/mcs/responses/endpoint"):
            return self.edr_response()

        logger.warn("unknown do_POST_mcs: %s", self.path)
        return self.ret("Unknown MCS command", code=500)

    def do_POST(self):
        assert os.getppid() == ORIGINAL_PPID
        ua = self.headers.get("User-Agent", "<unknown>").split('/', 1)[0]
        logger.debug("POST - %s (%s)", self.path, ua)
        logger.debug("RECV Cookie: %s", self.headers.get("Cookie", "<none>"))
        self.verify_cookies()

        if self.path == "/api/sessions":
            return self.zero()
        elif self.path.startswith("/mcs"):
            return self.do_POST_mcs()
        elif self.path == "/api/customer/flags":
            return self.retJson({})

        logger.warn("unknown do_POST: %s", self.path)
        message = "<html><body>POST REQUEST</body></html>"
        self.send_response(404, "POST Unknown")
        self.send_header("Content-Length", str(len(message)))
        self.sendAWSCookie()
        self.send_cookie()
        self.end_headers()
        self.wfile.write(message)
        return

    def mcs_update_status(self):
        mo = re.match(r"/mcs/statuses/endpoint/([^/]+)", self.path)
        if not mo:
            return self.ret("Bad status path", 400)

        eid = mo.group(1)
        endpoint = GL_ENDPOINTS.getEndpointByID(eid)
        if endpoint is None:
            ## Create endpoint?
            return self.ret("Status for unknown endpoint", 400)

        body = self.getBody()
        #~ logger.debug("BODY1 %s", body)
        doc = xml.dom.minidom.parseString(body)

        statusesNode = doc.getElementsByTagName("ns:statuses")[0]
        statusNodes = statusesNode.getElementsByTagName("status")

        for statusNode in statusNodes:
            appNode = statusNode.getElementsByTagName("appId")[0]
            app = getText(appNode)
            logger.debug("Received status with appID: {}".format(app))
            bodyNode = statusNode.getElementsByTagName("body")[0]
            body = getText(bodyNode)
            #~ body = xml.sax.saxutils.unescape(body)
            #~ logger.debug("BODY2 %s %s", app, body)
            endpoint.updateStatus(app, body)

        return self.ret("")

    def do_PUT_controller(self):
        global GL_ENDPOINTS
        if self.path.lower() == "/controller/mcs/policy":
            GL_ENDPOINTS.updatePolicy("MCS",self.getBody())
        elif self.path.lower() == "/controller/sav/policy":
            GL_ENDPOINTS.updatePolicy("SAV",self.getBody())
        elif self.path.lower() == "/controller/alc/policy":
            GL_ENDPOINTS.updatePolicy("ALC",self.getBody())
        elif self.path.lower() == "/controller/livequery/command":
            command_id = self.headers.get("Command-ID")
            GL_ENDPOINTS.setQuery("LiveQuery", self.getBody(), command_id)
        else:
            logger.warn("unknown do_PUT_controller: %s", self.path)
            message = "<html><body>UNKNOWN PUT REQUEST</body></html>"
            return self.ret(message, code=404)

        return self.retJson("")

    def do_PUT_mcs(self):
        global REREGISTER_NEXT
        ua = self.headers.get("User-Agent", "<unknown>").split('/', 1)[0]
        logger.debug("PUT - %s (%s); Cookie=%s", self.path, ua,
            self.headers.get("Cookie", "<none>"))
        self.verify_cookies()
        if self.path.startswith("/mcs/statuses/endpoint/"):
            if MCSRequestHandler.options.reregister or REREGISTER_NEXT:
                REREGISTER_NEXT = False
                return self.send_401()
            else:
                return self.mcs_update_status()

        logger.warn("unknown do_PUT_mcs: %s", self.path)
        message = "<html><body>UNKNOWN PUT REQUEST %s</body></html>"%self.path
        return self.ret(message, code=404)

    def do_PUT(self):
        assert os.getppid() == ORIGINAL_PPID
        if self.path.startswith("/mcs/"):
            return self.do_PUT_mcs()
        elif self.path.startswith("/controller/"):
            return self.do_PUT_controller()

        logger.warn("unknown do_PUT: %s", self.path)
        message = "<html><body>UNKNOWN PUT REQUEST</body></html>"
        return self.ret(message, code=404)

    def deleteCommands(self):
        mo = re.match(r"/mcs/commands/endpoint/([^/]+)/([^/]+)", self.path)
        if not mo:
            logger.warn("Unknown MCS delete command: %s", self.path)
            return self.ret("Unknown MCS delete command", code=500)
        GL_ENDPOINTS.deleteCommands(mo.group(1), mo.group(2).split(";"))
        return self.ret("")

    def do_DELETE(self):
        assert os.getppid() == ORIGINAL_PPID
        if self.path.startswith("/mcs/"):
            ua = self.headers.get("User-Agent", "<unknown>").split('/', 1)[0]
            logger.debug("DELETE - %s (%s)", self.path, ua)
            logger.debug("RECV Cookie: %s", self.headers.get("Cookie", "<none>"))
            self.verify_cookies()

            MCSRequestHandler.m_userAgent = self.headers.get("User-Agent", "<unknown>")

            if self.path.startswith("/mcs/commands/endpoint/"):
                return self.deleteCommands()

        logger.warn("unknown do_DELETE: %s", self.path)
        message = "<html><body>UNKNOWN DELETE REQUEST %s</body></html>"%self.path
        return self.ret(message, code=404)

    def version_string(self):
        return "MCS/localhost 1.0"


def getServerCert():
    basedir = getCloudAutomationDirectory()
    path = os.path.join(basedir, "server-private.pem")

    if os.path.isfile(path):
        return path
    try:
        subprocess.check_call(["make", "all"], cwd=basedir)
    except Exception:
        raise AssertionError("Certificate make failed - cannot continue with test.")

    assert os.path.isfile(path)
    return path


def daemonise():
    logger.info("Daemonising cloudServer %d", os.getpid())
    try:
        pid = os.fork()
        if pid > 0:
            # exit first parent
            sys.exit(0)
    except OSError as e:
        logger.fatal("fork #1 failed: %d (%s)\n", e.errno, e.strerror)
        sys.exit(1)

    os.setsid()

    # do second fork
    try:
        pid = os.fork()
        if pid > 0:
            # exit from second parent
            sys.exit(0)
    except OSError as e:
        logger.fatal("fork #2 failed: %d (%s)\n", e.errno, e.strerror)
        sys.exit(1)

    logger.info("Daemonised cloudServer %d", os.getpid())
    return os.getppid()


def runServer(options):
    port = int(options.port) if options.port else 4443
    logger.info("localhost CloudServer pid={} on port {}".format(os.getpid(), port))
    MCSRequestHandler.options = options
    MCSRequestHandler.m_userAgent = "<unknown>"

    httpd = http.server.HTTPServer(('localhost', port), MCSRequestHandler)
    certfile = getServerCert()
    logger.info("Cert path: %s", certfile)
    protocol = ssl.PROTOCOL_TLS
    if options.tls:
        protocol = options.tls
    logger.info("SSL version: %s", options.tls)
    httpd.socket = ssl.wrap_socket(httpd.socket, certfile=certfile, server_side=True, ssl_version=protocol)
    if options.daemon:
        daemonise()

    global ORIGINAL_PPID
    ORIGINAL_PPID = os.getppid()

    action_log.debug("START")
    try:
        if options.pidfile is not None:
            file(options.pidfile, b'w').write(b"%d\n" % os.getpid())
            httpd.timeout = 5
            while os.path.isfile(options.pidfile) and os.getppid() == ORIGINAL_PPID:
                httpd.handle_request()
        else:
            httpd.serve_forever()
    finally:
        action_log.debug("END")
        deletePidFile(options)


def parseArguments(parser=None):
    if not parser:
        parser = argparse.ArgumentParser(description='Cloud test server for SSPL Linux automation')
    ArgParserUtils.add_cloudserver_args(parser)
    options, _ = parser.parse_known_args()
    setDefaultPolicies(options)
    print(str(options))
    return options


def deletePidFile(options):
    if options is None:
        return
    if options.pidfile is None:
        return
    try:
        os.unlink(options.pidfile)
    except EnvironmentError:
        pass

def setDefaultPolicies(options):
    global INITIAL_ALC_POLICY
    global INITIAL_MCS_POLICY
    global INITIAL_SAV_POLICY
    global INITIAL_MDR_POLICY

    with open(options.INITIAL_ALC_POLICY) as alc_file:
        INITIAL_ALC_POLICY = alc_file.read()

    with open(options.INITIAL_MCS_POLICY) as mcs_file:
        INITIAL_MCS_POLICY = mcs_file.read()

    with open(options.INITIAL_SAV_POLICY) as sav_file:
        INITIAL_SAV_POLICY = sav_file.read()

    with open(options.INITIAL_MDR_POLICY) as mdr_file:
        INITIAL_MDR_POLICY = mdr_file.read()

def main(argv):
    try:
        options = parseArguments()
        setupLogging()

        global HEARTBEAT_ENABLED
        HEARTBEAT_ENABLED = options.heartbeat

        reset_cookies()
        runServer(options)
    except Exception as e:
        logger.warn("Exception from cloudServer.py: %s", e)
        return 1
    logger.info("Server stopped")
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

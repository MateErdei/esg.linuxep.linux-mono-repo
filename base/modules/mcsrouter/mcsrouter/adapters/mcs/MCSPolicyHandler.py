#!/usr/bin/env python

from __future__ import absolute_import,print_function,division,unicode_literals

import os

import xml.dom.minidom
import xml.parsers.expat # for xml.parsers.expat.ExpatError

import logging
logger = logging.getLogger(__name__)

import mcsrouter.utils.XmlHelper
import mcsrouter.utils.AtomicWrite
import mcsrouter.adapters.base.PolicyHandlerBase
import mcsrouter.utils.PathManager as PathManager

class MCSPolicyHandlerException(Exception):
    pass

class MCSPolicyHandler(mcsrouter.adapters.base.PolicyHandlerBase.PolicyHandlerBase):
    """
    Process MCS adapter policy
    as defined at
    https://wiki.sophos.net/display/SophosCloud/EMP%3A+policy-mcs
    """
    def __init__(self, installDir,
            policy_config,
            applied_config):
        super(MCSPolicyHandler,self).__init__(installDir)
        self.__m_compliance = None
        self.__m_policyXml = None
        PathManager.INST = installDir
        self.__m_policy_config = policy_config
        self.__m_applied_config = applied_config
        if not REGISTER_MCS:
            self.__loadPolicy()

    def policyName(self):
        return "MCS"

    def policyBaseName(self):
        return "MCS-25_policy.xml"

    def __policyPath(self):
        return os.path.join(PathManager.policyDir(),self.policyBaseName())

    def __savePolicy(self, policyXml=None, path=None):
        if policyXml is None:
            policyXml = self.__m_policyXml
        assert policyXml is not None
        if path is None:
            path = self.__policyPath()
        mcsrouter.utils.AtomicWrite.atomic_write(path, os.path.join(PathManager.tempDir(), self.policyBaseName()), policyXml)

    def __loadPolicy(self):
        path = self.__policyPath()
        if os.path.isfile(path):
            self.__m_policyXml = open(path,"rb").read()
            self.__tryApplyPolicy("existing",False)

    def __getElement(self, dom, elementName):
        nodes = dom.getElementsByTagName(elementName)
        if len(nodes) != 1:
            return None
        return nodes[0]


    def __applyPolicySetting(self, dom, policyOption, configOption=None, treatMissingAsEmpty=False):
        if configOption is None:
            configOption = policyOption

        node = self.__getElement(dom, policyOption)

        if node is None:
            if treatMissingAsEmpty:
                value = ""
            else:
                return False
        else:
            value = mcsrouter.utils.XmlHelper.getTextFromElement(node)

        logger.debug("MCS policy %s = %s",policyOption,value)

        self.__m_policy_config.set(configOption,value)
        return True

    def __applyPollingDelay(self, dom):
        node = self.__getElement(dom, "commandPollingDelay")
        if node is None:
            return False

        value = node.getAttribute("default")
        if value == "":
            logger.error("MCS policy commandPollingDelay has no attribute default")
            return False

        try:
            value = int(value)
        except ValueError:
            logger.error("MCS policy commandPollingDelay default is not a number")
            return False

        logger.debug("MCS policy commandPollingDelay = %d",value)
        self.__m_policy_config.set("COMMAND_CHECK_INTERVAL_MINIMUM",str(value))
        self.__m_policy_config.set("COMMAND_CHECK_INTERVAL_MAXIMUM",str(value))

        return True

    def __getNonEmptySubElements(self, node, subNodeName):

        subNodes = node.getElementsByTagName(subNodeName)

        texts = [ mcsrouter.utils.XmlHelper.getTextFromElement(x) for x in subNodes ]
        texts = [ s for s in texts if s != "" ]

        return texts


    def __applyMCSServer(self, dom):
        """
        We ignore multiple server nodes, since all the examples we've seen and the specification
        only have one server specified.
        """
        node = self.__getElement(dom, "servers")
        if node is None:
            return False

        servers = self.__getNonEmptySubElements(node,"server")

        if len(servers) == 0:
            logger.error("MCS Policy has no server nodes in servers element")
            return False

        index = 1
        for server in servers:
            key = "mcs_policy_url%d"%index
            logger.debug("MCS policy URL %s = %s",key,server)
            self.__m_policy_config.set(key,server)
            index += 1

        while True:
            key = "mcs_policy_url%d"%index
            if not self.__m_policy_config.remove(key):
                break
            index += 1
        return True

    def __applyMessageRelays(self, dom):
        """
        Reading message relay priority, port, address and ID from policy into config
        """
        node = self.__getElement(dom, "messageRelays")
        if node is None:
            messageRelays=[]
        else:
            messageRelays = node.getElementsByTagName("messageRelay")

        index = 1
        for messageRelay in messageRelays:
            priorityKey = "message_relay_priority%d"%index
            portKey = "message_relay_port%d"%index
            addressKey = "message_relay_address%d"%index
            idKey = "message_relay_id%d"%index

            priorityValue = messageRelay.getAttribute("priority")
            portValue = messageRelay.getAttribute("port")
            addressValue = messageRelay.getAttribute("address")
            idValue = messageRelay.getAttribute("id")

            logger.debug("MCS Policy Message Relay %s = %s",priorityKey,priorityValue)
            logger.debug("MCS policy Message Relay %s = %s",portKey,portValue)
            logger.debug("MCS policy Message Relay %s = %s",addressKey,addressValue)
            logger.debug("MCS policy Message Relay %s = %s",idKey,idValue)

            mrInfoSet = [priorityValue, portValue, addressValue, idValue]
            if None in mrInfoSet or "" in mrInfoSet:
                logger.error("Message Relay Policy is incomplete: %s", str(mrInfoSet))
                continue

            self.__m_policy_config.set(priorityKey,priorityValue)
            self.__m_policy_config.set(portKey,portValue)
            self.__m_policy_config.set(addressKey,addressValue)
            self.__m_policy_config.set(idKey,idValue)
            index += 1

        ## Remove old message relay config entries that may exist and haven't been overwritten
        while True:
            priorityKey = "message_relay_priority%d"%index
            portKey = "message_relay_port%d"%index
            addressKey = "message_relay_address%d"%index
            idKey = "message_relay_id%d"%index

            if not self.__m_policy_config.remove(addressKey):
                break
            self.__m_policy_config.remove(portKey)
            self.__m_policy_config.remove(priorityKey)
            self.__m_policy_config.remove(idKey)
            index += 1
        return True

    def __applyProxyOptions(self, dom):
        proxiesNode = self.__getElement(dom, "proxies")
        if proxiesNode is None:
            ## Don't apply credentials unless we have a proxy
            return False

        proxies = self.__getNonEmptySubElements(proxiesNode,"proxy")

        if len(proxies) == 0:
            logger.error("MCS Policy has no proxy nodes in proxies element")
            return False

        if len(proxies) > 1:
            logger.warning("Multiple MCS proxies in MCS policy")

        logger.debug("MCS policy proxy = %s",proxies[0])
        self.__m_policy_config.set("mcs_policy_proxy",proxies[0])

        credentialsNode = self.__getElement(dom,"proxyCredentials")
        if credentialsNode is None:
            return False

        credentials = self.__getNonEmptySubElements(credentialsNode,"credentials")

        if len(credentials) == 0:
            return False

        if len(credentials) > 1:
            logger.warning("Multiple MCS proxy credentials in MCS policy")

        logger.debug("MCS policy proxy credential = %s",credentials[0])
        self.__m_policy_config.set("mcs_policy_proxy_credentials",credentials[0])
        return True

    def __applyPolicy(self,policyAge,save):
        assert self.__m_policy_config is not None

        policyXml = self.__m_policyXml
        try:
            dom = xml.dom.minidom.parseString(policyXml)
        except xml.parsers.expat.ExpatError as e:
            logger.error("Failed to parse MCS policy (%s): %s", str(e), policyXml)
            return False
        except Exception:
            logger.exception("Failed to parse MCS policy: %s", policyXml)
            return False

        try:
            policyNodes = dom.getElementsByTagName("policy")
            if len(policyNodes) != 1:
                logger.error("MCS Policy doesn't have one policy node")
                return False

            policyNode = policyNodes[0]

            nodes = policyNode.getElementsByTagName("csc:Comp")
            if len(nodes) == 1:
                node = nodes[0]
                policyType = node.getAttribute("policyType")
                revID = node.getAttribute("RevID")
                compliance = (policyType, revID)
            else:
                logger.error("MCS Policy didn't contain one compliance node")
                compliance = None

            logger.info("Applying %s %s policy %s",policyAge,self.policyName(),compliance)

            ## Process policy options
            self.__applyPolicySetting(policyNode, "useSystemProxy")
            self.__applyPolicySetting(policyNode, "useAutomaticProxy")
            self.__applyPolicySetting(policyNode, "useDirect")
            self.__applyPollingDelay(policyNode)
            ## MCSToken is the config option we are already using for the Token elsewhere
            self.__applyPolicySetting(policyNode, "registrationToken", "MCSToken", treatMissingAsEmpty=True)
            self.__applyMCSServer(policyNode)
            self.__applyProxyOptions(policyNode)
            self.__applyMessageRelays(policyNode)

            ## Save configuration
            self.__m_policy_config.save()
            self.__m_compliance = compliance

            if save:
                ## Save successfully applied policy
                self.__savePolicy()
        finally:
            dom.unlink()

    def __tryApplyPolicy(self,policyAge,save):
        try:
            self.__applyPolicy(policyAge,save)
        except Exception as e:
            logger.exception("Failed to apply MCS policy")

    def process(self, policyXml):
        """
        Process a new policy
        """
        self.__m_policyXml = policyXml
        self.__tryApplyPolicy("new",True)

    def getPolicyInfo(self):
        """
        @return (policyType,RevID) or None
        """
        return self.__m_compliance

    def getCurrentMessageRelay(self):
        return self.__m_applied_config.getDefault("current_relay_id", None)

    def isCompliant(self):
        """
        @return True if active configuration matches policy settings
        """

        #~ self.__m_policy_config = policy_config
        #~ self.__m_applied_config = applied_config

        assert self.__m_applied_config is not None
        assert self.__m_policy_config is not None

        compliant = True

        for field in ("useSystemProxy","useAutomaticProxy","useDirect",
                      "MCSToken",
                      "mcs_policy_proxy",
                      "mcs_policy_proxy_credentials",
                      "COMMAND_CHECK_INTERVAL_MINIMUM",
                      "COMMAND_CHECK_INTERVAL_MAXIMUM"):
            if self.__m_policy_config.getDefault(field,None) != self.__m_applied_config.getDefault(field,None):
                logger.warning("MCS Policy not compliant: %s option differs",field)
                compliant = False

        ## Check URLS
        index = 1
        while True:
            field = "mcs_policy_url%d"%index
            policyValue = self.__m_policy_config.getDefault(field,None)
            appliedValue = self.__m_applied_config.getDefault(field,None)
            if policyValue != appliedValue:
                logger.warning("MCS Policy not compliant: %s option differs",field)
                compliant = False
            if policyValue is None and appliedValue is None:
                break
            index += 1

        return compliant

#~ <?xml version="1.0"?>
 #~ <policy xmlns:csc="com.sophos\msys\csc" type="mcs">
   #~ <meta protocolVersion="1.1"/>
   #~ <csc:Comp RevID="6f9cb63e8cb1eaa98df9efbf5d218056668f5f34392ba53cf206af03d7eb6614" policyType="25"/>
   #~ <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
     #~ <registrationToken>21e72d59248ce0dc5f957b659d72a4261b663041bbc96c54a81c8ea578186648</registrationToken>
     #~ <servers>
       #~ <server>https://mcs.sandbox.sophos/sophos/management/ep</server>
     #~ </servers>
     #~ <useSystemProxy>true</useSystemProxy>
     #~ <useAutomaticProxy>true</useAutomaticProxy>
     #~ <useDirect>true</useDirect>
     #~ <randomSkewFactor>1</randomSkewFactor>
     #~ <commandPollingDelay default="20"/>
     #~ <policyChangeServers/>
   #~ </configuration>
 #~ </policy>


#~ <?xml version="1.0"?>
#~ <policy xmlns:csc="com.sophos\msys\csc" type="mcs">
  #~ <meta protocolVersion="1.1"/>
  #~ <csc:Comp RevID="0e09e27cfc21f8d7510d562c56e34507711bdce48bfdfd3846965f130fef142a" policyType="25"/>
  #~ <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    #~ <registrationToken>1e64e66751cb985159cf40e8b06ca5018e5f7f5e0afad2bdc1225058b81a8b30</registrationToken>
    #~ <servers>
      #~ <server>https://dzr-mcs-amzn-eu-west-1-4c5f.upe.q.hmr.sophos.com/sophos/management/ep</server>
    #~ </servers>
    #~ <useSystemProxy>true</useSystemProxy>
    #~ <useAutomaticProxy>true</useAutomaticProxy>
    #~ <useDirect>true</useDirect>
    #~ <randomSkewFactor>1</randomSkewFactor>
    #~ <commandPollingDelay default="20"/>
    #~ <policyChangeServers/>
  #~ </configuration>
#~ </policy>


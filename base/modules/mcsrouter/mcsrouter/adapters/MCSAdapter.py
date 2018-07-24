
import AdapterBase

import xml.dom.minidom
import os
import json
import datetime

import logging
logger = logging.getLogger(__name__)

import mcs.MCSPolicyHandler
import mcsrouter.utils.PathManager as PathManager


TEMPLATE_STATUS_XML = """<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
 <ns:mcsStatus xmlns:ns="http://www.sophos.com/xml/mcs/status">
        <csc:CompRes xmlns:csc="com.sophos\msys\csc" Res="" RevID="" policyType="25" />
        <meta protocolVersion="1.0" timestamp="0" />
        <data><![CDATA[<?xml version="1.0" encoding="utf-8" ?><mcs xmlns="http://www.sophos.com/xml/mcs/status" agentVersion="1.0" />]]></data>
        <messageRelay lastUsed="" endpointId=""/>
        </ns:mcsStatus>
"""

class MCSAdapter(AdapterBase.AdapterBase):
    def __init__(self,
        installdir,
        policy_config,
        applied_config):
        if installdir is not None:
            PathManager.INST = installdir

        self.__m_policyHandler = mcs.MCSPolicyHandler.MCSPolicyHandler(
                    PathManager.installDir(),
                    policy_config,
                    applied_config)
        self.__m_relayId = None

    def getAppId(self):
        return "MCS"

    def __setCompliance(self, compNode):
        """
<csc:CompRes xmlns:csc="com.sophos\msys\csc" Res="" RevID="" policyType=""/>
        """
        policyInfo = self.__m_policyHandler.getPolicyInfo()
        if policyInfo is None:
            Res="NoRef"
            RevID=""
            policyType="25"
        else:
            policyType=policyInfo[0]
            RevID=policyInfo[1]
            if self.__m_policyHandler.isCompliant():
                Res="Same"
            else:
                Res="Diff"

        compNode.setAttribute("Res",Res)
        compNode.setAttribute("RevID",RevID)
        compNode.setAttribute("policyType",policyType)

    def _getStatusXml(self):
        doc = xml.dom.minidom.parseString(TEMPLATE_STATUS_XML)
        AdapterBase.remove_blanks(doc)

        compNode = doc.getElementsByTagName("csc:CompRes")[0]
        self.__setCompliance(compNode)

        # Add Message Relay to status XML.
        self.__m_relayId = self.__m_policyHandler.getCurrentMessageRelay()
        relayNode = doc.getElementsByTagName("messageRelay")[0]
        if self.__m_relayId:
            relayNode.setAttribute("endpointId",self.__m_relayId)
            relayNode.setAttribute("lastUsed",datetime.datetime.now().isoformat())
        else:
            relayNode.removeAttribute("endpointId")
            relayNode.removeAttribute("lastUsed")
        output = doc.toxml(encoding="utf-8")
        doc.unlink()
        logger.debug("Status MCS XML: %s"%output)
        return output

    def _hasNewStatus(self):
        relayId = self.__m_policyHandler.getCurrentMessageRelay()

        return relayId != self.__m_relayId

    def __processPolicy(self, policy):
        logger.debug("MCS Adapter processing policy %s",str(policy))
        try:
            self.__m_policyHandler.process(policy)
        except Exception as e:
            logger.exception("Exception while processing MCS policy")

        return []

    def __processAction(self, command):
        logger.error("Got action for the MCS adapter")
        return None

    def processCommand(self, command):
        try:
            logger.debug("MCS Adapter processing %s",str(command))
            try:
                policy = command.getPolicy()
                return self.__processPolicy(policy)
            except NotImplementedError:
                return self.__processAction(command)
        finally:
            command.complete()


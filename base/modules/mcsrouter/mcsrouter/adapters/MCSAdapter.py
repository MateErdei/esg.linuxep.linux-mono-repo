
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
                 install_dir,
                 policy_config,
                 applied_config):
        if install_dir is not None:
            PathManager.INST = install_dir

        self.__m_policy_handler = mcs.MCSPolicyHandler.MCSPolicyHandler(
            PathManager.install_dir(),
            policy_config,
            applied_config)
        self.__m_relay_id = None

    def get_app_id(self):
        return "MCS"

    def __set_compliance(self, comp_node):
        """
<csc:CompRes xmlns:csc="com.sophos\msys\csc" Res="" RevID="" policyType=""/>
        """
        policy_info = self.__m_policy_handler.get_policy_info()
        if policy_info is None:
            res = "NoRef"
            rev_id = ""
            policy_type = "25"
        else:
            policy_type = policy_info[0]
            rev_id = policy_info[1]
            if self.__m_policy_handler.is_compliant():
                res = "Same"
            else:
                res = "Diff"

        comp_node.setAttribute("Res", res)
        comp_node.setAttribute("RevID", rev_id)
        comp_node.setAttribute("policyType", policy_type)

    def _get_status_xml(self):
        doc = xml.dom.minidom.parseString(TEMPLATE_STATUS_XML)
        AdapterBase.remove_blanks(doc)

        comp_node = doc.getElementsByTagName("csc:CompRes")[0]
        self.__set_compliance(comp_node)

        # Add Message Relay to status XML.
        self.__m_relay_id = self.__m_policy_handler.get_current_message_relay()
        relay_node = doc.getElementsByTagName("messageRelay")[0]
        if self.__m_relay_id:
            relay_node.setAttribute("endpointId", self.__m_relay_id)
            relay_node.setAttribute(
                "lastUsed", datetime.datetime.now().isoformat())
        else:
            relay_node.removeAttribute("endpointId")
            relay_node.removeAttribute("lastUsed")
        output = doc.toxml(encoding="utf-8")
        doc.unlink()
        logger.debug("Status MCS XML: %s" % output)
        return output

    def _has_new_status(self):
        relay_id = self.__m_policy_handler.get_current_message_relay()

        return relay_id != self.__m_relay_id

    def __process_policy(self, policy):
        logger.debug("MCS Adapter processing policy %s", str(policy))
        try:
            self.__m_policy_handler.process(policy)
        except Exception as e:
            logger.exception("Exception while processing MCS policy")

        return []

    def __process_action(self, command):
        logger.error("Got action for the MCS adapter")
        return None

    def process_command(self, command):
        try:
            logger.debug("MCS Adapter processing %s", str(command))
            try:
                policy = command.get_policy()
                return self.__process_policy(policy)
            except NotImplementedError:
                return self.__process_action(command)
        finally:
            command.complete()

"""
mcs_adapter Module
"""

import datetime

import logging

import mcsrouter.adapters.adapter_base
import mcsrouter.adapters.mcs.mcs_policy_handler
import mcsrouter.utils.path_manager as path_manager
import mcsrouter.utils.xml_helper
from mcsrouter.utils.byte2utf8 import to_utf8



LOGGER = logging.getLogger(__name__)

# pylint: disable=anomalous-backslash-in-string
TEMPLATE_STATUS_XML = """<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
 <ns:mcsStatus xmlns:ns="http://www.sophos.com/xml/mcs/status">
        <csc:CompRes xmlns:csc="com.sophos\msys\csc" Res="" RevID="" policyType="25" />
        <meta protocolVersion="1.0" timestamp="0" />
        <data><![CDATA[<?xml version="1.0" encoding="utf-8" ?><mcs xmlns="http://www.sophos.com/xml/mcs/status" agentVersion="1.0" />]]></data>
        <messageRelay lastUsed="" endpointId=""/>
        </ns:mcsStatus>
"""


class MCSAdapter(mcsrouter.adapters.adapter_base.AdapterBase):
    """
    MCSAdapter class
    """

    def __init__(self,
                 install_dir,
                 policy_config,
                 applied_config):
        """
        __init__
        """
        if install_dir is not None:
            path_manager.INST = install_dir

        self.__m_policy_handler = mcsrouter.adapters.mcs.mcs_policy_handler.MCSPolicyHandler(
            path_manager.install_dir(), policy_config, applied_config)
        self.__m_relay_id = None
        self.app_id = "MCS"

    def get_app_id(self):
        """
        get_app_id
        """
        return self.app_id

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
        """
        _get_status_xml
        """
        doc = self._parse_xml_string(TEMPLATE_STATUS_XML)
        mcsrouter.adapters.adapter_base.remove_blanks(doc)

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
        LOGGER.debug("Status MCS XML: %s", output)
        output = output if isinstance(output, str) else to_utf8(output)
        return output

    def _has_new_status(self):
        """
        _has_new_status
        """
        relay_id = self.__m_policy_handler.get_current_message_relay()

        return relay_id != self.__m_relay_id

    def __process_policy(self, policy):
        """
        __process_policy
        """
        LOGGER.debug("MCS Adapter processing policy: {}".format(policy))
        self.__m_policy_handler.process(policy)
        return []

    def process_command(self, command):
        """
        process_command
        """
        try:
            LOGGER.debug("MCS Adapter processing %s", str(command))
            try:
                policy = command.get_policy()
                return self.__process_policy(policy)
            except NotImplementedError:
                # If we start processing actions in MCS we should add a method
                # to this class to process it. For now we log an error.
                LOGGER.error("Got action for the MCS adapter: %s", str(command))
                return None
        finally:
            command.complete()

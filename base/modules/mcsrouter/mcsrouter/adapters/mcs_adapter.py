# Copyright 2019 Sophos Plc, Oxford, England.
"""
mcs_adapter Module
"""

import datetime
import logging
import os
import stat

import mcsrouter.adapters.adapter_base
import mcsrouter.adapters.mcs.mcs_policy_handler
import mcsrouter.utils.path_manager as path_manager
import mcsrouter.utils.xml_helper
from mcsrouter.utils.xml_helper import toxml_utf8
from mcsrouter.mcsclient.mcs_exception import MCSException

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

class FailedToProcessActionException(MCSException):
    pass

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
        self.__m_last_policy = None

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
        output = toxml_utf8(doc)
        doc.unlink()
        LOGGER.debug("Status MCS XML: {}".format(output))
        return output

    def _has_new_status(self):
        """
        _has_new_status
        """
        relay_id = self.__m_policy_handler.get_current_message_relay()

        return relay_id != self.__m_relay_id

    def get_last_policy(self):
        """
        get_last_policy
        """
        return self.__m_last_policy

    def __process_policy(self, policy):
        """
        __process_policy
        """
        if policy == self.__m_last_policy:
            LOGGER.info(f"Policy is identical to last one - will not be processed by {self.app_id} Adapter")
            return []
        self.__m_last_policy = policy

        LOGGER.info("MCS Adapter processing policy: {}".format(policy))
        self.__m_policy_handler.process(policy)
        return []

    def process_command(self, command):
        """
        process_command
        """
        try:
            command_str = str(command)
            LOGGER.debug("MCS Adapter processing %s", command_str)
            policy = command.get_policy()
            return self.__process_policy(policy)
        finally:
            command.complete()

    def _write_action_to_tmp(self, action_name, body):
        action_path_tmp = os.path.join(path_manager.actions_temp_dir(), action_name)
        mcsrouter.utils.filesystem_utils.utf8_write(action_path_tmp, body)

        # Make sure that action is group readable so that Management Agent (or plugins) can read the file.
        os.chmod(action_path_tmp, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP )
        return action_path_tmp

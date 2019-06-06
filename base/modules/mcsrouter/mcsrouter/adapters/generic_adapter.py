"""
generic_adapter Module
"""

import xml.dom.minidom
import os
import logging

import mcsrouter.adapters.adapter_base
import mcsrouter.utils.atomic_write
import mcsrouter.utils.timestamp
import mcsrouter.utils.path_manager as path_manager
import mcsrouter.utils.xml_helper as xml_helper


LOGGER = logging.getLogger(__name__)


class GenericAdapter(mcsrouter.adapters.adapter_base.AdapterBase):
    """
    GenericAdapter class
    """

    def __init__(self, app_id, install_dir=None):
        """
        __init__
        """
        self.__m_app_id = app_id
        self.__m_last_status_time = None
        if install_dir is not None:
            path_manager.INST = install_dir

    def get_app_id(self):
        """
        get_app_id
        """
        return self.__m_app_id

    def __process_policy(self, policy):
        """
        __process_policy
        """
        # handle non ascii characters ( LINUXEP-6757 )
        policy = policy.encode('utf-8')
        LOGGER.debug(
            "%s Adapter processing policy %s",
            self.__m_app_id,
            policy)
        LOGGER.debug("Received %s policy", self.__m_app_id)

        try:
            doc = self._parse_xml_string(policy)
        except xml.parsers.expat.ExpatError as exception:
            LOGGER.error(
                "Failed to parse %s policy (%s): %s",
                self.__m_app_id,
                str(exception),
                policy)
            return []

        nodes = doc.getElementsByTagName("csc:Comp")
        if len(nodes) == 1:
            node = nodes[0]
            policy_type = node.getAttribute("policyType")
            policy_name = "%s-%s_policy.xml" % (self.__m_app_id, policy_type)
        else:
            LOGGER.info(
                "%s Policy didn't contain one compliance node",
                self.__m_app_id)
            policy_name = "%s_policy.xml" % (self.__m_app_id)

        policy_path = os.path.join(path_manager.policy_dir(), policy_name)
        policy_path_tmp = os.path.join(path_manager.temp_dir(), policy_name)
        mcsrouter.utils.atomic_write.atomic_write(
            policy_path, policy_path_tmp, policy)

        return []

    def __process_action(self, command):
        """
        __process_action
        """
        LOGGER.debug("Received %s action", self.__m_app_id)

        body = command.get(u"body")
        try:
            timestamp = command.get(u"creationTime")
        except KeyError:
            timestamp = mcsrouter.utils.timestamp.timestamp()

        action_name = "%s_action_%s.xml" % (self.__m_app_id, timestamp)
        action_path = os.path.join(path_manager.action_dir(), action_name)
        action_path_tmp = os.path.join(path_manager.temp_dir(), action_name)
        mcsrouter.utils.atomic_write.atomic_write(
            action_path, action_path_tmp, body)

        return []

    def _get_status_xml(self):
        """
        _get_status_xml
        """
        status_path = os.path.join(
            path_manager.status_dir(),
            "%s_status.xml" %
            self.__m_app_id)
        try:
            self.__m_last_status_time = os.path.getmtime(status_path)
        except OSError:
            pass
        try:
            statusXml = xml_helper.get_escaped_non_ascii_content(
                status_path)
        except IOError:
            return None
        try:
            xml_helper.check_xml_has_no_script_tags(statusXml)
        except RuntimeError:
            return None

        return statusXml

    def _has_new_status(self):
        """
        _has_new_status
        """
        status_path = os.path.join(
            path_manager.status_dir(),
            "%s_status.xml" %
            self.__m_app_id)
        try:
            status_time = os.path.getmtime(status_path)
        except OSError:
            return False

        if self.__m_last_status_time is None:
            return True

        return status_time > self.__m_last_status_time

    def process_command(self, command):
        """
        process_command
        """
        try:
            LOGGER.debug(
                "%s Adapter processing %s",
                self.__m_app_id,
                str(command))
            try:
                policy = command.get_policy()
                return self.__process_policy(policy)
            except NotImplementedError:
                return self.__process_action(command)
        finally:
            command.complete()

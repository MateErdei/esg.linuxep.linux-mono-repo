# Copyright 2019 Sophos Plc, Oxford, England.
"""
generic_adapter Module
"""

import datetime
import logging
import os
import xml.dom.minidom

import mcsrouter.adapters.adapter_base
import mcsrouter.utils.atomic_write
import mcsrouter.utils.path_manager as path_manager
import mcsrouter.utils.timestamp
import mcsrouter.utils.utf8_write
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
        assert isinstance(app_id, str)
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
        try:
            policy = policy.encode('utf-8')
        except AttributeError as exception:
            # If connection is lost product system test has shown (timing dependant) that the policy.encode line
            # will fail because it is a byte object.  Hopefully catching the error will help identify why.
            LOGGER.error(
                "Failed to encode {} policy, error: ({}), using policy value: {}".format(
                    self.__m_app_id,
                    str(exception),
                    policy))
            return []

        LOGGER.debug(
            "{} Adapter processing policy {}".format(
                self.__m_app_id,
                policy)
        )
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
            policy_name = "%s_policy.xml" % (self.__m_app_id)

        policy_path_tmp = os.path.join(path_manager.policy_temp_dir(), policy_name)
        mcsrouter.utils.utf8_write.utf8_write(policy_path_tmp, policy)

        return []

    def __process_action(self, command):
        """
        __process_action
        """
        LOGGER.debug("Received %s action", self.__m_app_id)

        body = command.get("body")
        try:
            timestamp = command.get("creationTime")
        except KeyError:
            timestamp = mcsrouter.utils.timestamp.timestamp()

        order_tag = datetime.datetime.now().strftime("%Y%d%m%H%M%S%f")
        action_name = "{}_{}_action_{}.xml".format(order_tag, self.__m_app_id, timestamp)
        action_path_tmp = os.path.join(path_manager.actions_temp_dir(), action_name)
        mcsrouter.utils.utf8_write.utf8_write(action_path_tmp, body)
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
            status_xml = xml_helper.get_escaped_non_ascii_content(
                status_path)
        except IOError:
            return None
        try:
            xml_helper.check_xml_has_no_script_tags(status_xml)
        except RuntimeError:
            return None

        return status_xml

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

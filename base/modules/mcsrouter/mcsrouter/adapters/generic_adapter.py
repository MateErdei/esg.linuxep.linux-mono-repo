# Copyright 2020 Sophos Plc, Oxford, England.
"""
generic_adapter Module
"""

import datetime
import json
import logging
import os
import time
import calendar
import re
import xml.dom.minidom
import stat

import mcsrouter.adapters.adapter_base
import mcsrouter.utils.filesystem_utils
import mcsrouter.utils.path_manager as path_manager
import mcsrouter.utils.timestamp
import mcsrouter.utils.xml_helper as xml_helper
from mcsrouter.mcsclient.mcs_exception import MCSException

LOGGER = logging.getLogger(__name__)

class FailedToProcessActionException(MCSException):
    pass

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
        self.__m_last_policy = None
        if install_dir is not None:
            path_manager.INST = install_dir

    def get_app_id(self):
        """
        get_app_id
        """
        return self.__m_app_id

    def get_last_policy(self):
        """
        get_last_policy
        """
        return self.__m_last_policy

    def __process_policy(self, policy):
        """
        word of caution: this is a 'private method' will not be overriden by classes inheriting from GenericAdapter
        """
        if policy == self.__m_last_policy:
            LOGGER.debug(f"Policy is identical to last one - will not be processed by {self.__m_app_id} Adapter")
            return []
        self.__m_last_policy = policy

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
        except (xml.parsers.expat.ExpatError, ValueError) as exception:
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
        mcsrouter.utils.filesystem_utils.utf8_write(policy_path_tmp, policy)

        # Make sure that policy is group readable so that Management Agent (or plugins) can read the file.
        os.chmod(policy_path_tmp, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP)
        return []

    def _get_action_name(self, command):
        try:
            timestamp = command.get("creationTime")
        except KeyError:
            timestamp = mcsrouter.utils.timestamp.timestamp()
        order_tag = datetime.datetime.now().strftime("%Y%m%d%H%M%S%f")
        return "{}_{}_action_{}.xml".format(order_tag, self.__m_app_id, timestamp)

    def _get_action_name_json(self, command):
        app_id = self.get_app_id()
        LOGGER.debug("{} received an action".format(app_id))

        correlation_id = command.get("id")
        try:
            timestamp = command.get("creationTime")
        except KeyError as ex:
            LOGGER.error("Could not get creationTime, using the timestamp. Exception: {}".format(ex))
            timestamp = mcsrouter.utils.timestamp.timestamp()
        ttl = self._convert_ttl_to_epoch_time(timestamp, command.get("ttl"))
        order_tag = datetime.datetime.now().strftime("%Y%m%d%H%M%S%f")
        return f"{order_tag}_{app_id}_{correlation_id}_request_{timestamp}_{ttl}.json"

    def _is_json(self, command):
        try:
            body = command.get("body")
        except KeyError:
            return False
        try:
            json.loads(body)
        except (json.JSONDecodeError):
            return False
        return True

    def _process_action(self, command):
        """
        Process the actions by creating the file
        Can be overridden by classes inheriting from Generic Adapter
        """
        LOGGER.debug("Received %s action", self.get_app_id())

        body = command.get("body")
        if self._is_json(command):
            action_name = self._get_action_name_json(command)
        else:
            action_name = self._get_action_name(command)

        self._write_tmp_action(action_name, body)
        LOGGER.debug(f"{self.get_app_id()} action saved to path {action_name}")
        return []

    def _write_tmp_action(self, action_name, body):
        action_path_tmp = os.path.join(path_manager.actions_temp_dir(), action_name)
        mcsrouter.utils.filesystem_utils.utf8_write(action_path_tmp, body)

        # Make sure that action is group readable so that Management Agent (or plugins) can read the file.
        os.chmod(action_path_tmp, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP )
        return action_path_tmp

    def _get_status_xml(self):
        """
        _get_status_xml
        """
        status_path = os.path.join(
            path_manager.status_dir(),
            "%s_status.xml" %
            self.__m_app_id)
        if not os.path.isfile(status_path):
            return None
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
                LOGGER.debug("{} adapter try to process policy".format(self.__m_app_id))
                policy = command.get_policy()
                return self.__process_policy(policy)
            except NotImplementedError:
                LOGGER.debug("{} adaptor processing as action".format(self.__m_app_id))
                return self._process_action(command)
            except FailedToProcessActionException as exception:
                LOGGER.error(f"Failed to process {self.__m_app_id} action, reason: {exception}")
        finally:
            command.complete()

    def _convert_ttl_to_epoch_time(self, timestamp, ttl):
        """
        :param timestamp: The time the command was created by central
        :param ttl: the time to live value given with the command
        :return: the unix time in seconds that the command should be considered to have expired
        """
        match_object = re.match("^PT([0-9]+)S$", ttl)
        if match_object:
            seconds_to_live = int(match_object.group(1))
        else:
            LOGGER.warning(f"TTL of command is in an invalid format: {ttl}. Using default of 10000 seconds")
            seconds_to_live = 10000
        expected_timestamp_format = "%Y-%m-%dT%H:%M:%S.%fZ"
        try:
            epoch_time = calendar.timegm(datetime.datetime.strptime(timestamp, expected_timestamp_format).timetuple())
        except ValueError as error:
            LOGGER.error(f"Failed to convert creation time: '{timestamp}' to unix time: {error}. Using system time")
            epoch_time = time.time()

        return int(epoch_time + seconds_to_live)

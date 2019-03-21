
import xml.dom.minidom
import os
import json
import datetime
import time

import logging
logger = logging.getLogger(__name__)

import AdapterBase
import mcsrouter.utils.AtomicWrite
import mcsrouter.utils.Timestamp
import mcsrouter.utils.PathManager as PathManager
import mcsrouter.utils.XmlHelper as XmlHelper


class GenericAdapter(AdapterBase.AdapterBase):
    def __init__(self, app_id, install_dir=None):
        self.__m_app_id = app_id
        self.__m_last_status_time = None
        if install_dir is not None:
            PathManager.INST = install_dir

    def get_app_id(self):
        return self.__m_app_id

    def __process_policy(self, policy):
        # handle non ascii characters ( LINUXEP-6757 )
        policy = policy.encode('utf-8')
        logger.debug(
            "%s Adapter processing policy %s",
            self.__m_app_id,
            policy)
        logger.debug("Received %s policy", self.__m_app_id)

        try:
            doc = xml.dom.minidom.parseString(policy)
        except xml.parsers.expat.ExpatError as e:
            logger.error(
                "Failed to parse %s policy (%s): %s",
                self.__m_app_id,
                str(e),
                policy)
            return []
        except Exception:
            logger.exception(
                "Failed to parse %s policy: %s",
                self.__m_app_id,
                policy)
            return []

        nodes = doc.getElementsByTagName("csc:Comp")
        if len(nodes) == 1:
            node = nodes[0]
            policy_type = node.getAttribute("policyType")
            policy_name = "%s-%s_policy.xml" % (self.__m_app_id, policy_type)
        else:
            logger.info(
                "%s Policy didn't contain one compliance node" %
                self.__m_app_id)
            policy_name = "%s_policy.xml" % (self.__m_app_id)

        policy_path = os.path.join(PathManager.policy_dir(), policy_name)
        policy_path_tmp = os.path.join(PathManager.temp_dir(), policy_name)
        mcsrouter.utils.AtomicWrite.atomic_write(
            policy_path, policy_path_tmp, policy)

        return []

    def __process_action(self, command):
        logger.debug("Received %s action", self.__m_app_id)

        body = command.get(u"body")
        try:
            timestamp = command.get(u"creationTime")
        except KeyError:
            timestamp = mcsrouter.utils.Timestamp.timestamp()

        action_name = "%s_action_%s.xml" % (self.__m_app_id, timestamp)
        action_path = os.path.join(PathManager.action_dir(), action_name)
        action_path_tmp = os.path.join(PathManager.temp_dir(), action_name)
        mcsrouter.utils.AtomicWrite.atomic_write(
            action_path, action_path_tmp, body)

        return []

    def _get_status_xml(self):
        status_path = os.path.join(
            PathManager.status_dir(),
            "%s_status.xml" %
            self.__m_app_id)
        try:
            self.__m_last_status_time = os.path.getmtime(status_path)
        except OSError:
            pass
        try:
            return XmlHelper.get_xml_file_content_with_escaped_non_ascii_code(
                status_path)
        except IOError:
            return None

    def _has_new_status(self):
        status_path = os.path.join(
            PathManager.status_dir(),
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
        try:
            logger.debug(
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

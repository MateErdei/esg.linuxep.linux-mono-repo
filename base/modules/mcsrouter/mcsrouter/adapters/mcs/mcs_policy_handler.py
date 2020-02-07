#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
mcs_policy_handler Module
"""

import logging
import os
import xml.dom.minidom
import xml.parsers.expat  # for xml.parsers.expat.ExpatError

import mcsrouter.utils.path_manager as path_manager
import mcsrouter.utils.utf8_write
import mcsrouter.utils.xml_helper

LOGGER = logging.getLogger(__name__)


class MCSPolicyHandlerException(Exception):
    """
    MCSPolicyHandlerException
    """
    pass


class MCSPolicyHandler:
    """
    Process MCS adapter policy
    as defined at
    https://wiki.sophos.net/display/SophosCloud/EMP%3A+policy-mcs
    """
    # pylint: disable=no-self-use

    def __init__(self, install_dir,
                 policy_config,
                 applied_config):
        self.__m_compliance = None
        self.__m_policy_xml = None
        path_manager.INST = install_dir
        self.__m_policy_config = policy_config
        self.__m_applied_config = applied_config
        #REGISTER_MCS is set in register_central.py or mcs_router.py
        if not REGISTER_MCS:  # pylint: disable=undefined-variable
            self.__load_policy()

    def policy_name(self):
        """
        policy_name
        """
        return "MCS"

    def policy_base_name(self):
        """
        policy_base_name
        """
        return "MCS-25_policy.xml"

    def __policy_path(self):
        """
        __policy_path
        """
        return os.path.join(path_manager.policy_dir(), self.policy_base_name())

    def __save_policy(self, policy_xml=None, path=None):
        """
        __save_policy
        """
        if policy_xml is None:
            policy_xml = self.__m_policy_xml
        assert policy_xml is not None
        if path is None:
            path = self.__policy_path()

        policy_path_tmp = os.path.join(path_manager.policy_temp_dir(), self.policy_base_name())
        mcsrouter.utils.utf8_write.utf8_write(policy_path_tmp, policy_xml)

    def __load_policy(self):
        """
        __load_policy
        """
        path = self.__policy_path()
        if os.path.isfile(path):
            self.__m_policy_xml = open(path, "rb").read()
            self.__try_apply_policy("existing", False)

    def __get_element(self, dom, element_name):
        """
        __get_element
        """
        # in sspl dom has been converted from an elementTree object to a dom.
        # using an elementTree we can perform the xpath findall("./mcs:configuration/mcs:" + elementName,...)
        # as is done in SAV.
        # The change below is mimicking this search as xpath not available for dom object.

        parent_node_name = "configuration"
        nodes = dom.getElementsByTagName(element_name)

        valid_nodes = []
        for node in nodes:
            if node.parentNode.tagName == parent_node_name:
                valid_nodes.append(node)

        if len(valid_nodes) == 1:
            return valid_nodes[0]

        return None

    def __get_default_int_value(self, dom, element_name, default_value):
        node = self.__get_element(dom, element_name)
        if node is None:
            LOGGER.error(
                "MCS policy has no element {}".format(element_name))
            return default_value

        value = node.getAttribute("default")
        if value == "":
            LOGGER.error(
                "MCS policy {} has no attribute default".format(element_name))
            return default_value

        try:
            value = int(value)
        except ValueError:
            LOGGER.error(
                "MCS policy {} default is not a number".format(element_name))
            return default_value

        return value

    def __get_int_value(self, dom, element_name, default_value):
        node = self.__get_element(dom, element_name)
        if node is None:
            LOGGER.error(
                "MCS policy has no element {}".format(element_name))
            return default_value
        value = mcsrouter.utils.xml_helper.get_text_from_element(node)

        try:
            value = int(value)
        except ValueError:
            LOGGER.error(
                "MCS policy {} default is not a number".format(element_name))
            return default_value

        return value

    def __apply_policy_setting(
            self,
            dom,
            policy_option,
            config_option=None,
            treat_missing_as_empty=False):
        """
        __apply_policy_setting
        """
        if config_option is None:
            config_option = policy_option

        node = self.__get_element(dom, policy_option)

        if node is None:
            if treat_missing_as_empty:
                value = ""
            else:
                return False
        else:
            value = mcsrouter.utils.xml_helper.get_text_from_element(node)

        LOGGER.debug("MCS policy %s = %s", policy_option, value)

        self.__m_policy_config.set(config_option, value)
        return True

    def __apply_polling_delay(self, dom):
        """
        __apply_polling_delay
        """
        min_poll_value = str(self.__get_default_int_value(dom, "commandPollingDelay", 20))
        #TODO LINUXDAR-1412 set COMMAND_CHECK_INTERVAL_MAXIMUM to flagsPollingInterval
        #max_poll_value = str(self.__get_default_int_value(dom, "flagsPollingInterval", 4*3600))
        max_poll_value = min_poll_value
        ping_timeout = str(self.__get_int_value(dom, "pushPingTimeout", 90))
        push_poll_value = str(self.__get_int_value(dom, "pushFallbackPollInterval", int(min_poll_value)))


        LOGGER.debug("MCS policy commandPollingDelay = {}".format(min_poll_value))
        self.__m_policy_config.set(
            "COMMAND_CHECK_INTERVAL_MINIMUM", min_poll_value)
        self.__m_policy_config.set(
            "COMMAND_CHECK_INTERVAL_MAXIMUM", max_poll_value)
        self.__m_policy_config.set(
            "PUSH_SERVER_CHECK_INTERVAL", push_poll_value)
        self.__m_policy_config.set(
            "PUSH_SERVER_CONNECTION_TIMEOUT", ping_timeout)

        return True

    def __get_non_empty_sub_elements(self, node, sub_node_name):
        """
        __get_non_empty_sub_elements
        """

        sub_nodes = node.getElementsByTagName(sub_node_name)

        texts = [mcsrouter.utils.xml_helper.get_text_from_element(
            x) for x in sub_nodes]
        texts = [s for s in texts if s != ""]

        return texts

    def __apply_mcs_server(self, dom):
        """
        load mcs server urls into policy config
        """
        node = self.__get_element(dom, "servers")
        if node is None:
            return False

        servers = self.__get_non_empty_sub_elements(node, "server")
        
        if not servers:
            LOGGER.error("MCS Policy has no server nodes in servers element")
            return False

        index = 1
        for server in servers:
            key = "mcs_policy_url%d" % index
            LOGGER.debug("MCS policy URL %s = %s", key, server)
            self.__m_policy_config.set(key, server)
            index += 1

        # removes old servers that are no longer in the policy
        while True:
            key = "mcs_policy_url%d" % index
            if not self.__m_policy_config.remove(key):
                break
            index += 1
        return True

    def __apply_push_server(self, dom):
        """
        load push server urls into policy config
        """
        node = self.__get_element(dom, "pushServers")
        if node is None:
            return False

        servers = self.__get_non_empty_sub_elements(node, "pushServer")

        if not servers:
            LOGGER.error("MCS Policy has no pushServer nodes in PushServers element")
            return False

        index = 1
        for server in servers:
            key = "pushServer%d" % index
            LOGGER.debug("Push Server URL %s = %s", key, server)
            self.__m_policy_config.set(key, server)
            index += 1

        # removes old servers that are no longer in the policy
        while True:
            key = "pushServer%d" % index
            if not self.__m_policy_config.remove(key):
                break
            index += 1
        return True

    def __apply_message_relays(self, dom):
        """
        Reading message relay priority, port, address and ID from policy into config
        """
        node = self.__get_element(dom, "messageRelays")
        if node is None:
            message_relays = []
        else:
            message_relays = node.getElementsByTagName("messageRelay")

        index = 1
        for message_relay in message_relays:
            priority_key = "message_relay_priority%d" % index
            port_key = "message_relay_port%d" % index
            address_key = "message_relay_address%d" % index
            id_key = "message_relay_id%d" % index

            priority_value = message_relay.getAttribute("priority")
            port_value = message_relay.getAttribute("port")
            address_value = message_relay.getAttribute("address")
            id_value = message_relay.getAttribute("id")

            LOGGER.debug(
                "MCS Policy Message Relay %s = %s",
                priority_key,
                priority_value)
            LOGGER.debug(
                "MCS policy Message Relay %s = %s",
                port_key,
                port_value)
            LOGGER.debug(
                "MCS policy Message Relay %s = %s",
                address_key,
                address_value)
            LOGGER.debug("MCS policy Message Relay %s = %s", id_key, id_value)

            message_relay_info_set = [
                priority_value, port_value, address_value, id_value]
            if None in message_relay_info_set or "" in message_relay_info_set:
                LOGGER.error(
                    "Message Relay Policy is incomplete: %s",
                    str(message_relay_info_set))
                continue

            self.__m_policy_config.set(priority_key, priority_value)
            self.__m_policy_config.set(port_key, port_value)
            self.__m_policy_config.set(address_key, address_value)
            self.__m_policy_config.set(id_key, id_value)
            index += 1

        # Remove old message relay config entries that may exist and haven't
        # been overwritten
        while True:
            priority_key = "message_relay_priority%d" % index
            port_key = "message_relay_port%d" % index
            address_key = "message_relay_address%d" % index
            id_key = "message_relay_id%d" % index

            if not self.__m_policy_config.remove(address_key):
                break
            self.__m_policy_config.remove(port_key)
            self.__m_policy_config.remove(priority_key)
            self.__m_policy_config.remove(id_key)
            index += 1
        return True

    def __apply_proxy_options(self, dom):
        """
        __apply_proxy_options
        """
        proxies_node = self.__get_element(dom, "proxies")

        def cleanup_proxy():
            self.__m_policy_config.remove("mcs_policy_proxy")
            self.__m_policy_config.remove("mcs_policy_proxy_credentials")

        if proxies_node is None:
            ## Remove any existing proxy configuration
            cleanup_proxy()
            return False

        proxies = self.__get_non_empty_sub_elements(proxies_node, "proxy")

        if not proxies:
            LOGGER.error("MCS Policy has no proxy nodes in proxies element")
            ## Remove any existing proxy configuration
            cleanup_proxy()
            return False

        if len(proxies) > 1:
            LOGGER.warning("Multiple MCS proxies in MCS policy")

        LOGGER.debug("MCS policy proxy = %s", proxies[0])
        self.__m_policy_config.set("mcs_policy_proxy", proxies[0])

        credentials_node = self.__get_element(dom, "proxyCredentials")
        if credentials_node is None:
            return False

        credentials = self.__get_non_empty_sub_elements(
            credentials_node, "credentials")

        if not credentials:
            return False

        if len(credentials) > 1:
            LOGGER.warning("Multiple MCS proxy credentials in MCS policy")

        LOGGER.debug("MCS policy proxy credential = %s", credentials[0])
        self.__m_policy_config.set(
            "mcs_policy_proxy_credentials",
            credentials[0])
        return True

    def __apply_policy(self, policy_age, save):
        """
        __apply_policy
        """
        assert self.__m_policy_config is not None

        policy_xml = self.__m_policy_xml
        try:
            dom = mcsrouter.utils.xml_helper.parseString(policy_xml)
        except xml.parsers.expat.ExpatError as exception:
            message = "Failed to parse MCS policy ({}): {}".format(str(exception), policy_xml)
            LOGGER.error(message)
            raise RuntimeError("Invalid xml policy")

        try:
            policy_nodes = dom.getElementsByTagName("policy")
            if len(policy_nodes) != 1:
                LOGGER.error("MCS Policy doesn't have one policy node")
                raise mcsrouter.utils.xml_helper.XMLException("Rejecting policy")

            policy_node = policy_nodes[0]

            nodes = policy_node.getElementsByTagName("csc:Comp")
            if len(nodes) == 1:
                node = nodes[0]

                policy_type = node.getAttribute("policyType")
                if policy_type == "":
                    LOGGER.error("MCS policy did not contain policy type")
                    raise mcsrouter.utils.xml_helper.XMLException("Rejecting policy")

                rev_id = node.getAttribute("RevID")
                if rev_id == "":
                    LOGGER.error("MCS policy did not contain revID")
                    raise mcsrouter.utils.xml_helper.XMLException("Rejecting policy")

                compliance = (policy_type, rev_id)
            else:
                LOGGER.error("MCS Policy didn't contain one compliance node")
                raise mcsrouter.utils.xml_helper.XMLException("Rejecting policy")


            LOGGER.info(
                "Applying %s %s policy %s",
                policy_age,
                self.policy_name(),
                compliance)

            # Process policy options
            self.__apply_policy_setting(policy_node, "useSystemProxy")
            self.__apply_policy_setting(policy_node, "useAutomaticProxy")
            self.__apply_policy_setting(policy_node, "useDirect")
            self.__apply_policy_setting(policy_node, "pushPingTimeout")
            self.__apply_policy_setting(policy_node, "pushFallbackPollInterval")
            self.__apply_polling_delay(policy_node)
            # MCSToken is the config option we are already using for the Token
            # elsewhere
            self.__apply_policy_setting(
                policy_node,
                "registrationToken",
                "MCSToken",
                treat_missing_as_empty=True)
            self.__apply_mcs_server(policy_node)
            self.__apply_push_server(policy_node)
            self.__apply_proxy_options(policy_node)
            self.__apply_message_relays(policy_node)

            # Save configuration
            self.__m_policy_config.save()
            self.__m_compliance = compliance

            if save:
                # Save successfully applied policy
                self.__save_policy()

        # except mcsrouter.utils.xml_helper.XMLException:
        #     return

        finally:
            dom.unlink()

    def __try_apply_policy(self, policy_age, save):
        """
        __try_apply_policy
        """
        try:
            self.__apply_policy(policy_age, save)
        except Exception as ex:  # pylint: disable=broad-except
            LOGGER.error("Failed to apply MCS policy: {}".format(ex))

    def process(self, policy_xml):
        """
        Process a new policy
        """
        self.__m_policy_xml = policy_xml
        self.__try_apply_policy("new", True)

    def get_policy_info(self):
        """
        @return (policy_type,RevID) or None
        """
        return self.__m_compliance

    def get_current_message_relay(self):
        """
        get_current_message_relay
        """
        return self.__m_applied_config.get_default("current_relay_id", None)

    def is_compliant(self):
        """
        @return True if active configuration matches policy settings
        """

        #~ self.__m_policy_config = policy_config
        #~ self.__m_applied_config = applied_config

        assert self.__m_applied_config is not None
        assert self.__m_policy_config is not None

        compliant = True

        for field in ("useSystemProxy", "useAutomaticProxy", "useDirect",
                      "MCSToken",
                      "mcs_policy_proxy",
                      "mcs_policy_proxy_credentials",
                      "COMMAND_CHECK_INTERVAL_MINIMUM",
                      "COMMAND_CHECK_INTERVAL_MAXIMUM"):
            if self.__m_policy_config.get_default(field, None)\
                    != self.__m_applied_config.get_default(field, None):
                LOGGER.warning("MCS Policy not compliant: %s option differs", field)
                compliant = False

        # Check URLS
        index = 1
        while True:
            field = "mcs_policy_url%d" % index
            policy_value = self.__m_policy_config.get_default(field, None)
            applied_value = self.__m_applied_config.get_default(field, None)
            if policy_value != applied_value:
                LOGGER.warning(
                    "MCS Policy not compliant: %s option differs", field)
                compliant = False
            if policy_value is None and applied_value is None:
                break
            index += 1

        return compliant

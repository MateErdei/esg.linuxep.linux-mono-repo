#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
mcs_policy_handler Module
"""

import logging
import os
import stat
import xml.dom.minidom
import xml.parsers.expat  # for xml.parsers.expat.ExpatError

import mcsrouter.utils.path_manager as path_manager
import mcsrouter.utils.default_values as default_values
import mcsrouter.utils.filesystem_utils
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
        mcsrouter.utils.filesystem_utils.utf8_write(policy_path_tmp, policy_xml)
        os.chmod(policy_path_tmp, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP)

    def __load_policy(self):
        """
        __load_policy
        """
        path = self.__policy_path()
        if os.path.isfile(path):
            self.__m_policy_xml = open(path, "rb").read()
            self.__try_apply_policy("existing", False)

    def __get_all_matching_elements(self, dom, element_name):
        parent_node_name = "configuration"
        nodes = dom.getElementsByTagName(element_name)

        valid_nodes = []
        for node in nodes:
            if node.parentNode.tagName == parent_node_name:
                valid_nodes.append(node)

        if len(valid_nodes) > 0:
            return valid_nodes

        return None

    def __get_default_int_value(self, dom, element_name, default_value):
        nodes = self.__get_all_matching_elements(dom, element_name)
        if nodes is None:
            LOGGER.warning(f"MCS policy has no element {element_name}")
            return default_value

        if len(nodes) != 1:
            LOGGER.debug(f"MCS policy has multiple elements with name {element_name}, using default value")
            return default_value

        value = nodes[0].getAttribute("default")
        if value == "":
            LOGGER.error(f"MCS policy {element_name} has no attribute default")
            return default_value

        try:
            if not value.isdigit():
                raise ValueError

            value = int(value)
        except ValueError:
            LOGGER.error(
                "MCS policy {} default is not a number".format(element_name))
            return default_value

        return value

    def __get_int_value(self, dom, element_name, default_value):
        nodes = self.__get_all_matching_elements(dom, element_name)
        if nodes is None:
            LOGGER.info(
                "MCS policy has no element {}".format(element_name))
            return default_value

        if len(nodes) != 1:
            LOGGER.debug(f"MCS policy has multiple elements with name {element_name}, using default value")
            return default_value

        value = mcsrouter.utils.xml_helper.get_text_from_element(nodes[0])

        try:
            if not value.isdigit():
                raise ValueError

            value = int(value)
        except ValueError:
            LOGGER.error(
                f"MCS policy {element_name} default is not a number")
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
        if not config_option:
            config_option = policy_option

        nodes = self.__get_all_matching_elements(dom, policy_option)

        if not nodes:
            if treat_missing_as_empty:
                value = ""
            else:
                return False
        else:
            if len(nodes) > 1:
                LOGGER.debug(f"When applying policy setting for '{policy_option}', multiple elements were matched. "
                             f"Using first match and ignoring others.")
            value = mcsrouter.utils.xml_helper.get_text_from_element(nodes[0])

        LOGGER.debug("MCS policy %s = %s", policy_option, value)

        self.__m_policy_config.set(config_option, value)
        return True

    def __apply_polling_delay(self, dom):
        """
        __apply_polling_delay
        """
        command_polling_delay = self.__get_default_int_value(dom, "commandPollingDelay",
                                                      default_values.get_default_command_poll())
        command_polling_delay = str(min(command_polling_delay, default_values.get_max_for_any_value()))


        flags_polling_interval = self.__get_default_int_value(dom, "flagsPollingInterval",
                                                      default_values.get_default_flags_poll())
        flags_polling_interval = str(min(flags_polling_interval, default_values.get_max_for_any_value()))


        push_ping_timeout = self.__get_int_value(dom, "pushPingTimeout",
                                            default_values.get_default_push_ping_timeout())
        push_ping_timeout = str(min(push_ping_timeout, default_values.get_max_for_any_value()))


        push_fallback_poll_interval = self.__get_int_value(dom, "pushFallbackPollInterval", int(command_polling_delay))
        push_fallback_poll_interval = str(min(push_fallback_poll_interval, default_values.get_max_for_any_value()))

        LOGGER.debug("MCS policy commandPollingDelay = {}".format(command_polling_delay))
        self.__m_policy_config.set(
            "commandPollingDelay", command_polling_delay)
        self.__m_policy_config.set(
            "flagsPollingInterval", flags_polling_interval)
        self.__m_policy_config.set(
            "pushPingTimeout", push_ping_timeout)
        self.__m_policy_config.set(
            "pushFallbackPollInterval", push_fallback_poll_interval)

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
        nodes = self.__get_all_matching_elements(dom, "servers")
        if nodes is None:
            return False
        if len(nodes) > 1:
            LOGGER.debug(f"When applying policy setting for 'servers', multiple elements were matched. "
                         f"Using first match and ignoring others.")

        servers = self.__get_non_empty_sub_elements(nodes[0], "server")
        
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
        # clear the list of push servers in the config before updating with list from policy
        old_push_server_index = 1
        while True:
            key = "pushServer%d" % old_push_server_index
            if not self.__m_policy_config.remove(key):
                break
            old_push_server_index += 1

        nodes = self.__get_all_matching_elements(dom, "pushServers")
        if nodes is None:
            return False

        if len(nodes) > 1:
            LOGGER.debug(f"When applying policy setting for 'pushServers', multiple elements were matched. "
                         f"Using first match and ignoring others.")

        servers = self.__get_non_empty_sub_elements(nodes[0], "pushServer")

        if not servers:
            LOGGER.info("MCS Policy has no pushServer nodes in PushServers element")
            return False

        index = 1
        for server in servers:
            key = "pushServer%d" % index
            LOGGER.debug("Push Server URL %s = %s", key, server)
            self.__m_policy_config.set(key, server)
            index += 1
        return True

    def __apply_message_relays(self, dom):
        """
        Reading message relay priority, port, address and ID from policy into config
        """
        nodes = self.__get_all_matching_elements(dom, "messageRelays")
        if nodes is None:
            message_relays = []
        else:
            message_relays = nodes[0].getElementsByTagName("messageRelay")

        if len(nodes) > 1:
            LOGGER.debug(f"When applying policy setting for 'servers', multiple elements were matched. "
                         f"Using first match and ignoring others.")

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
        def cleanup_proxy():
            self.__m_policy_config.remove("mcs_policy_proxy")
            self.__m_policy_config.remove("mcs_policy_proxy_credentials")

        def get_sub_element(policy_dom, element, sub_element):
            proxies_nodes = self.__get_all_matching_elements(policy_dom, element)
            if proxies_nodes:
                if len(proxies_nodes) > 1:
                    LOGGER.debug(f"When applying policy setting for '{element}', multiple elements were matched. "
                                 f"Using first match and ignoring others.")
                return self.__get_non_empty_sub_elements(proxies_nodes[0], sub_element)
            return None

        ## Remove any existing proxy configuration
        cleanup_proxy()

        proxies = get_sub_element(policy_dom=dom, element="proxies", sub_element="proxy")
        if proxies:
            if len(proxies) > 1:
                LOGGER.warning("Multiple MCS proxies in MCS policy")

            LOGGER.debug("MCS policy proxy = %s", proxies[0])
            self.__m_policy_config.set("mcs_policy_proxy", proxies[0])
        else:
            LOGGER.info("MCS Policy has no proxy nodes in proxies element")
            return False

        credentials = get_sub_element(policy_dom=dom, element="proxyCredentials", sub_element="credentials")
        if credentials:
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
        assert self.__m_policy_config

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
            self.__apply_policy_setting(policy_node, "customerId", treat_missing_as_empty=True)
            self.__apply_policy_setting(policy_node, "deviceId", config_option="policy_device_id",
                                        treat_missing_as_empty=True)
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

        except mcsrouter.utils.xml_helper.XMLException:
            return

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

    def apply_policy(self, policy_xml):
        self.__m_policy_xml = policy_xml
        self.__apply_policy("new", True)

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
                      "commandPollingDelay",
                      "flagsPollingInterval"):
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
